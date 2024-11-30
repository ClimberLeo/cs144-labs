Checkpoint 2 Writeup
====================

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [6] hours to do. I [did/did not] attend the lab session.

## 实现思路

在TCP协议中，两方或“对等体”参与TCP连接，每个对等体同时充当“Sender”(对于自己将要传出的字节流)和“Receiver”(对于将要传入的字节流)。
本次lab实现了Receiver端，它的具体工作就是接受从对等端Sender发送过来的信息，将其放入到之前实现的Reassembler中，并发送回执信息给对等端Sender。lab的本身实现并不困难，但引入了一些网络协议栈设计中很重要的思想
首先由于TCP数据段中头部的空间很宝贵（网络传输对性能极其敏感，数据量的细微变动会对性能产生很大的影响），因此为了节省空间，将原本需要64位才能完整表示的序列号改成了32位数，但由于32位数最大只能代表4GiB的空间（对于当今的网络数据量来说，并不算大），因此lab中设计了一个Wrap32类来包装32位数，使其可以在节省空间的同时保持数据信息的完整性。具体而言，wrap操作通过简单的取模来将一个64位数转为32位类型，而unwrap操作通过找到当前32位数取模前的所有可能数中与当前首个unreassemble的索引最接近的64数来将其还原，这种设计十分的巧妙。结合了reassembler的特性和对网络传输性能的考虑。
关于收发端的消息内容：sender发送的数据主要包含数据载荷（可能为空），序列号seqno，SYN标识（如果设置表示发送的seqno就是ISN），FIN标识；receiver回复的数据主要有ackno（表示当前首个unreassemble的seqno），窗口大小（当前接收端可承受的最大字节数），另外收发端都有一个错误表示RST，当某一段发生错误想要关闭连接时，它会被设为true
其次TCP中很重要的一个机制就是流量控制，发送端需要了解当前接收端可以承受的最大数据量（字节数），receiver回复消息中的“窗口大小”就提供了此项信息，这使得sender可以根据当前网络状况实时调整发送的载荷大小，避免数据包过大导致的网络拥塞、丢包等问题，最大化保持数据传输的顺畅。
另外，TCP协议中有三种确定数据位置的信息，比较容易混淆：
+ `seqno`: 数据的序列号，它的起始位置SYN的序列号（ISN）是随机生成的，这是TCP为了提高健壮性并避免被属于相同终端节点之间早期连接的旧segment混淆，同时确保序列号无法被猜到并且不太可能重复而设置的机制，除了表示S“零点”的SYN的标识外，他还有一个结束标识FIN，这两个标识独占两个序列号，不存储数据，这点需要注意，比如SYN是0，FIN是5，那么实际数据从1开始，到4结束。seq的数据范围只限于无符号32位，这意味着在超出uint范围时它会从0重新开始（会在这个范围内循环）
+ `absolute seqno`: 绝对序列号，相当于将起始索引SYN固定为了0，但它的数据范围变成了uint64
+ `stream indices`: reassembler用于识别数据切片在整个数据流中的位置索引，它也默认从0开始，但没有SYN和FIN，从它到absolute seqno的转换由receiver负责，其中的一些实现细节也已体现在了实现代码中

Describe Wrap32 and TCPReceiver structure and design. [Describe data
structures and approach taken. Describe alternative designs considered
or tested.  Describe benefits and weaknesses of your design compared
with alternatives -- perhaps in terms of simplicity/complexity, risk
of bugs, asymptotic performance, empirical performance, required
implementation time and difficulty, and other factors. Include any
measurements if applicable.]

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

- Optional: I made an extra test I think will be helpful in catching bugs: [describe where to find]
