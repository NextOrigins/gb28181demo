#ifndef ARLIVE_THREAD_LOCAL_OBJECT_H
#define ARLIVE_THREAD_LOCAL_OBJECT_H

#include "rtc_base/thread.h"
#include "rtc_base/location.h"

#include <functional>
#include <memory>

namespace arlive {

template <typename T>
class ThreadLocalObject {
public:
	template <
		typename Generator,
		typename = std::enable_if_t<std::is_same<T*, decltype(std::declval<Generator>()())>::value>>
	ThreadLocalObject(rtc::Thread *thread, Generator &&generator) :
	_thread(thread),
	_valueHolder(std::make_unique<ValueHolder>()) {
		assert(_thread != nullptr);
		_thread->PostTask(RTC_FROM_HERE, [valueHolder = _valueHolder.get(), generator = std::forward<Generator>(generator)]() mutable {
			valueHolder->_value.reset(generator());
		});
	}

	~ThreadLocalObject() {
		_thread->PostTask(RTC_FROM_HERE, [valueHolder = std::move(_valueHolder)](){
			valueHolder->_value.reset();
		});
	}

	template <typename FunctorT>
	void perform(const rtc::Location& posted_from, FunctorT &&functor) {
		_thread->PostTask(posted_from, [valueHolder = _valueHolder.get(), f = std::forward<FunctorT>(functor)]() mutable {
			assert(valueHolder->_value != nullptr);
			f(valueHolder->_value.get());
		});
	}

	T *getSyncAssumingSameThread() {
		assert(_thread->IsCurrent());
		assert(_valueHolder->_value != nullptr);
		return _valueHolder->_value.get();
	}

private:
	struct ValueHolder {
		std::shared_ptr<T> _value;
	};

	rtc::Thread *_thread = nullptr;
	std::unique_ptr<ValueHolder> _valueHolder;

};

} // namespace arlive

#endif
