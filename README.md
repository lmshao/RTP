# RTP
Implementation of a RTP server that sends video stream (H.264/HEVC) using the Realtime Transfer Protocol (RTP).

### Receive & Play Video as RTP Client

**1 .FFmpeg/ffplay**

play rtp video stream.
```sh
ffplay -protocol_whitelist "file,rtp,udp" play.sdp
```

send video file as RTP server

```sh
ffmpeg -re -i Sample.h264 -vcodec copy -f rtp rtp://127.0.0.1:1234 > play.sdp
```


**2. VLC**

Open **play.sdp** using VLC before RTPServer runs.
