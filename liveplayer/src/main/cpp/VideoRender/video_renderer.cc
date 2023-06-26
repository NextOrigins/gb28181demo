/*
 *  Copyright (c) 2013 The WebRTC@AnyRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "video_renderer.h"

// TODO(pbos): Android renderer

namespace webrtc {

class NullRenderer : public VideoRenderer {
	void OnFrame(const webrtc::VideoFrame& video_frame) override {}
};

VideoRenderer* VideoRenderer::Create(const void* hwnd,
                                     size_t width,
                                     size_t height) {
  VideoRenderer* renderer =  NULL;
  if (renderer != NULL) {
    // TODO(mflodman) Add a warning log.
    return renderer;
  }

  return new NullRenderer();
}
}  // namespace webrtc
