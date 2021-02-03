# FFmpeg 之音视频同步

- [FFmpeg 之音视频同步](#ffmpeg-之音视频同步)
  - [初识时间线](#初识时间线)

## 初识时间线

时间线，或者说播放器如何知道在正确的时间来播放每一帧。

再上一个例子中，我们可以在这看到我们保留了一些帧：

![frame 0](./img/hello_world_frames/frame0.png)
![frame 1](./img/hello_world_frames/frame1.png)
![frame 2](./img/hello_world_frames/frame2.png)
![frame 3](./img/hello_world_frames/frame3.png)
![frame 4](./img/hello_world_frames/frame4.png)
![frame 5](./img/hello_world_frames/frame5.png)

当设计一个播放器时，在合适的时间**播放每一帧**很重要，否则音视频不同步会造成很严重的观看体验。

因此我们需要一些策略能平滑的播放每一帧。所以每一帧都有一个播放时间戳（PTS），PTS 是一个持续增长的数字，可以通过一个时间基数除以 **帧率（fps）** 来获得。

比如 `fps=60/1`，`timebase=1/60000`，`timescale=1/timebase`，每一个 PTS 的增长 `timescale  / fps = 1000`，因此每一帧 PTS 的时间如下（假设开始为0）:

- `frame=0, PTS = 0, PTS_TIME = 0`
- `frame=1, PTS = 1000, PTS_TIME = PTS * timebase = 0.016`
- `frame=2, PTS = 2000, PTS_TIME = PTS * timebase = 0.033`

几乎相同的场景，我们把 timebase 改成了 1/60。

- `frame=0, PTS = 0, PTS_TIME = 0`
- `frame=1, PTS = 1, PTS_TIME = PTS * timebase = 0.016`
- `frame=2, PTS = 2, PTS_TIME = PTS * timebase = 0.033`
- `frame=3, PTS = 3, PTS_TIME = PTS * timebase = 0.050`

例如 fps=25，timebase=1/75，PTS 的增长将会是 timescale / fps = 3，如下：

- `frame=0, PTS = 0, PTS_TIME = 0`
- `frame=1, PTS = 3, PTS_TIME = PTS * timebase = 0.04`
- `frame=2, PTS = 6, PTS_TIME = PTS * timebase = 0.08`
- `frame=3, PTS = 9, PTS_TIME = PTS * timebase = 0.12`
- ...
- `frame=24, PTS = 72, PTS_TIME = PTS * timebase = 0.96`
- ...
- `frame=4064, PTS = 12192, PTS_TIME = PTS * timebase = 162.56`

现在通过 `pts_time` 我们找到一个方式去同步音频的 `pts_time`。FFmpeg libav 提供了如下接口：

- fps = AVStream->avg_frame_rate
- tbr = AVStream->r_frame_rate
- tbn = AVStream->time_base

注意上节示例程序的输出信息：

```shell
~/WorkSpace/NoteBook_FFmpegLearning/LibFFmpegUsingExample LibFFmpegUsing*
❯ ./build/HelloWorld ./small_bunny_1080p_60fps.mp4
LOG: initializing all the containers, codecs and protocols.
LOG: opening the input file (./small_bunny_1080p_60fps.mp4) and loading format (container) header
LOG: format mov,mp4,m4a,3gp,3g2,mj2, duration 2022000 us, bit_rate 0
LOG: finding stream info from format
LOG: AVStream->time_base before open coded 1/15360
LOG: AVStream->r_frame_rate before open coded 60/1
LOG: AVStream->start_time 0
LOG: AVStream->duration 30720
LOG: finding the proper decoder (CODEC)
LOG: AVPacket->pts 0
LOG: AVPacket->pts 1024
LOG: AVPacket->pts 512
LOG: Frame 1 (type=I, size=100339 bytes, format=0) pts 0 key_frame 1 [DTS 0]
LOG: AVPacket->pts 256
LOG: Frame 2 (type=B, size=7484 bytes, format=0) pts 256 key_frame 0 [DTS 3]
LOG: AVPacket->pts 768
LOG: Frame 3 (type=B, size=14764 bytes, format=0) pts 512 key_frame 0 [DTS 2]
LOG: AVPacket->pts 2048
LOG: Frame 4 (type=B, size=7058 bytes, format=0) pts 768 key_frame 0 [DTS 4]
LOG: AVPacket->pts 1536
LOG: Frame 5 (type=P, size=37353 bytes, format=0) pts 1024 key_frame 0 [DTS 1]
LOG: AVPacket->pts 1280
LOG: Frame 6 (type=B, size=8678 bytes, format=0) pts 1280 key_frame 0 [DTS 7]
LOG: releasing all the resources
```

可以发现，我们的编码顺序 DTS 是（帧：1,6,4,2,3,5），但是我们的播放顺序是（帧：1,2,3,4,5）。同时我们可以看到 B 帧相对于 P 帧和 I 帧是比较节约空间的。

[I、P、B 以及 DTS 、PTS 解析](https://www.cnblogs.com/yongdaimi/p/10676309.html)
