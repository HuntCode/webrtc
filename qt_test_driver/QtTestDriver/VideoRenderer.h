#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include "api/media_stream_interface.h"
#include "api/video/video_frame.h"

#include <functional>
#include <mutex>

class VideoRenderer : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
 public:
  using FrameCallback = std::function<void(const webrtc::VideoFrame&)>;

  explicit VideoRenderer();
  ~VideoRenderer();

  void SetTrack(webrtc::VideoTrackInterface* track);

  // 设置帧处理回调
  void SetFrameCallback(FrameCallback callback);

  void OnFrame(const webrtc::VideoFrame& frame) override;

 private:
  std::mutex callback_mutex_;
  FrameCallback frame_callback_;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> track_;
};

#endif  // VIDEO_RENDERER_H
