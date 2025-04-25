#include "VideoRenderer.h"

VideoRenderer::VideoRenderer() {}

VideoRenderer::~VideoRenderer() {}

void VideoRenderer::SetTrack(webrtc::VideoTrackInterface* track) {
    rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track(track);

    if (track_) {
        track_->RemoveSink(this);
    }
    track_ = rendered_track;
    if (track_) {
      track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
    }
}

void VideoRenderer::SetFrameCallback(FrameCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    frame_callback_ = std::move(callback);
}

void VideoRenderer::OnFrame(const webrtc::VideoFrame& frame) {
    FrameCallback callback_copy;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        callback_copy = frame_callback_;
    }

    if (callback_copy) {
      callback_copy(frame);
    }
}
