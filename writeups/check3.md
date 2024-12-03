Checkpoint 3 Writeup
====================

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This checkpoint took me about [n] hours to do. I [did/did not] attend the lab session.

## 实现思路

本次要实现的是TCP的**Sender**，在上一次lab中提到过，由于TCP是在两个Peer之间建立的全连接双向通信，因此每个Peer既充当Sender又充当Receiver。
而Sender的主要功能是负责接收应用程序以字节流的形式传过来的数据，将该字节流分割成一个个的segment（其大小由Receiver端的窗口大小决定，这也是实现流量控制的关键），加上该segment对应的头部信息（序列号，必要时还要加上SYN/FIN），通过网络发送到对端的Receiver，每次发送时只要窗口大小还够，就应尽可能的多发送字节流中的数据（按一个个段），窗口大小的计算要考虑到已经发送但还未收到确认的数据大小，因为它们后续可能要重传。
由于TCP保证数据的**可靠传输**，因此需要确保发送的数据已经完整的被对端接收，具体而言，TCP实现了一个**超时重传**机制，记录每个segment的发送时间，若超出限定时间（这个时间的初值由lab测试作为参数传入）未收到对端的确认，则重传该segment（至于处理乱序信息和重复信息，则由Receiver端的Reassembler来处理，Sender只需要保证对端接收到每个字节至少一次）。
超时重传机制的关键在于发送端如何识别哪些被发送过的segment需要重传，这类segment应该具备这样的特征：从被发送开始在规定时间内未收到对端对当前seqno的ack，因此我的实现策略如下（lab指导表述得有些云里雾里，因此我的实现基于了对指导的理解以及一些自己的思考）：设计了一个timer，每次发送数据时若未启动则将其启动（用当前的时间戳作为启动时间，并添加过期时间），每次调用tick（这个函数相当于模拟了时间流逝的过程）时检查定时器是否过期，若过期则发送最早发送且未被确认的segment，并重新启动timer。每次接收到新的ackno时，将其所覆盖的所有数据段全部移除队列，表示其已被确认，同时若还有数据未被确认则重启timer，否则停止timer。
最后还有一些细枝末节，这里就直接按照测试用例的指示来写了，比如收到了0的窗口大小时要将其设置为1（这样做是有实际意义的，因为receiver是在每次接收到消息后才发送回执消息报告窗口大小，这样将窗口大小设为1，每次只发送一个字节的数据，虽然在receiver窗口已满时不会收到其回执的有效ackno，但可以不断实时的询问窗口大小，以便后续调整）以便后续发送询问消息，但不应用指数退避策略。还有一些判断是否可以加FIN等逻辑细节此处不再赘述。

Program Structure and Design of the TCPSender [Describe data
structures and approach taken. Describe alternative designs considered
or tested.  Describe benefits and weaknesses of your design compared
with alternatives -- perhaps in terms of simplicity/complexity, risk
of bugs, asymptotic performance, empirical performance, required
implementation time and difficulty, and other factors. Include any
measurements if applicable.]: []

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
