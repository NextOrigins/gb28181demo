#include "VideoCaptureInterface.h"

#include "VideoCaptureInterfaceImpl.h"

namespace arlive {

std::unique_ptr<VideoCaptureInterface> VideoCaptureInterface::Create(
   std::shared_ptr<Threads> threads,
   std::shared_ptr<PlatformContext> platformContext) {
	return std::make_unique<VideoCaptureInterfaceImpl>(platformContext, std::move(threads));
}

VideoCaptureInterface::~VideoCaptureInterface() = default;

} // namespace ARLIVE
