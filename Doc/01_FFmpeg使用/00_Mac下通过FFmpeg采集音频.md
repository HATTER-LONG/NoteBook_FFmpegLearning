# Mac 下使用 FFmpeg 采集音频

- [Mac 下使用 FFmpeg 采集音频](#mac-下使用-ffmpeg-采集音频)
  - [Swift 调用 C 接口](#swift-调用-c-接口)
  - [引入 FFmpeg 库](#引入-ffmpeg-库)
  - [FFmpeg 采集音频](#ffmpeg-采集音频)
  - [控制录制开关](#控制录制开关)

[本节源码](../../LibFFmpegUsingExample/Src/01_FFmpegUsingExample/01_FFmpegWithAudioBaseMac.c)

## Swift 调用 C 接口

1. 编写 C 接口程序。

    ```cpp
    #ifndef testFFmpeg_h
    #define testFFmpeg_h

    #include <stdio.h>
    void testFuncInC(void);
    #endif /* testFFmpeg_h */
    ```

2. 添加桥接头文件。

    ```cpp
    //
    //  Use this file to import your target's public headers that you would like to expose to Swift.
    //

    #import "testFFmpeg.h"
    ```

3. Swift 中调用。

    ```swift
    @objc
    func myfunc(){
        testFuncInC()
        
    }
    ```

## 引入 FFmpeg 库

1. 将 ffmpeg 库文件及头文件拷贝到工程目录中。

    ```shell
    # pwd 项目目录
    mkdir include
    mkdir libs
    cp -r /usr/local/Cellar/ffmpeg/4.3.1_9/include/* ./include
    cp -r /usr/local/Cellar/ffmpeg/4.3.1_9/lib/* ./libs
    ```

2. 引入库文件和头文件。
    - General --> FrameWorks..  ->  添加库
    - Build Settings  -->  User Header Search Paths  -->  Release Debug 添加头文件路径
    - 这样在代码中就可以使用引入的库了。

    ```c
    #include "FFmpegUsing.h"
    #include "libavutil/avutil.h"

    void TestInC(int input)
    {
        printf("Hello World %d\n", input);
        av_log_set_level(AV_LOG_DEBUG);
        av_log(NULL, AV_LOG_DEBUG, "Hello, World!\n");
        return;
    }
    ```

3. 关闭沙箱，使得程序可以访问工程以外的文件。
    - Signing & Capabilities 中删除 Box 相关。

## FFmpeg 采集音频

1. 打开输入设备：
   - 注册设备：avdevice_register_all。
   - 设置采集方式：Mac（avfoundation）、Windows（dshow）、Linux（alsa）。
   - 打开音频设备：avdevice_register_all。

    ```c
    AVFormatContext *fmt_ctx = NULL;
    AVDictionary *options = NULL;
    char errors[1024] = {0};
    //[[video device]:[audio device]] :0 默认 mic 音频
    char *devicename = ":0";
    //rigister audio device
    avdevice_register_all();
    //get format
    AVInputFormat * iformat = av_find_input_format("avfoundation");
    //open device
    av_log_set_level(AV_LOG_DEBUG);
    int ret = avformat_open_input(&fmt_ctx, devicename, iformat, &options);
    if(ret < 0)
    {
        av_strerror(ret, errors, 1024);
        av_log(NULL, AV_LOG_DEBUG, "Failed to open audio device, [%d]%s\n", ret, errors);
        return;
    }
    ```

2. 获取数据包：
   - 读取数据：av_read_frame。
   - 暂存数据：AVPacket，两个重要成员 data 数据、size 数据大小。
   - 相关 API：av_init_packet（初始化 AVPacket，除了 data 与 size 以外的成员初始化）、av_packet_unref（解引用 AVPacket）、av_packet_alloc（AVPacket 堆分配空间，并用 init 进行初始化）、av_packet_free（unref 解引用，使用 free 释放 alloc 申请的内存）。

    ```c
    AVPacket pkt;
    av_init_packet(&pkt);
    int count = 0;
    while(((ret = av_read_frame(fmt_ctx, &pkt)) == 0) && count++ < 500 )
    {
        printf("pkt size is %d\n", pkt.size);
    }
    av_packet_unref(&pkt);
    ```

3. 输出文件。
    - 通过 ffplay 播放： ffplay -ar 44100 -ac 2 -f f32le audio.pcm

## 控制录制开关

界面为主线程，控制录音开启与关闭，创建一条线程取进行音频采集。

```swift
import Cocoa

class ViewController: NSViewController {
    
    var recStatus: Bool = false;
    var thread: Thread?
    let btn = NSButton.init(title:"", target:nil, action:nil)

    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Do any additional setup after loading the view.
        self.view.setFrameSize(NSSize(width: 320, height: 240))
        
        btn.title = "开始录制"
        btn.bezelStyle = .rounded
        btn.setButtonType(.pushOnPushOff)
        btn.frame = NSRect(x:320/2-60,y: 240/2-15, width:120, height: 30)
        btn.target = self
        btn.action = #selector(myfunc)
        
        self.view.addSubview(btn)

        // Do any additional setup after loading the view.
    }
    
    @objc func myfunc(){
        //print("callback!")
        self.recStatus = !self.recStatus;
        
        if recStatus {
            thread = Thread.init(target: self,
                                 selector: #selector(self.recAudio),
                                 object: nil)
            thread?.start()
            
            self.btn.title = "停止录制"
        }else{
            set_status(0);
            self.btn.title = "开始录制"
        }
        //rec_audio()
    }
    
    @objc func recAudio(){
        //print("start thread")
        rec_audio()
    }

    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
}
```
