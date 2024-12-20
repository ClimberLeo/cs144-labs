用时：6小时

## lab总结与具体实现思路

此次实验实现了位于链路层的网络接口，它是连接 IP 层和链路层的桥梁，负责将从上层接收到的 IP 数据包封装成以太网帧，并通过 TAP 设备（这个设备负责与 Linux 内核交互，可以将其看作网络接口和 Linux 内核之间的一个中间层，在实际应用中，这个设备允许应用程序直接与链路层交互）将帧发送到物理网络接口（如以太网卡）从而在网络中传输，每个网络接口可以有一个或多个 IP 地址，通常情况下只会有一个 MAC 地址（但有时利用虚拟化等技术也可以拥有多个 MAC地址）。他同时还负责将从物理接口接收到的以太网帧解封装成 IP 数据包转发给 IP 层（或解析出 ARP 请求并进行响应）。在具体实现上，直接在实验中定义好的相关类（如 `InternetDatagram`, `EthernetFrame` 等）上进行相关操作即可，逻辑并不复杂。
本实验还了同样位于链路层中的一个重要协议：ARP，它是地址解析协议，用于解析和缓存当前主机到达本地局域网中其他主机的 IP 地址与 MAC 地址的映射关系，以便本机根据目标主机的 IP 地址快速查找其 MAC 地址以发送数据包。具体实现上，这里选用了 `unordered_map` 来存储 IP 地址到 MAC 地址的映射关系，同时额外维护一个时间戳，来表示当前映射记录是否超时（对于每条记录最多维护30秒），朴素的判断超时需要在每次时间戳变更时遍历所有映射记录判断，这样效率很低，因此这里用到了一个**惰性删除**的小技巧，只在每次发送数据帧时查看待发送的映射记录（如果存在）是否超时，若超时，则将该记录删除，并按该记录不存在时的情况来处理当前数据帧（即发送 arp 请求等待响应）。
发送数据报时，若存在映射记录，则可以直接封装链路帧并发送。否则需要发送 arp 请求，并将数据报维护在一个未发送队列中，这里依然使用了 `unordered_map` 来维护，主要是为了方便维护目标 IP 地址相同的所有数据报（相同 IP 下的数据报维护在一个队列中），且这里同样使用了时间戳，用以避免同一 IP 地址频繁发送 ARP 请求（限制在 5 秒内最多发送一次）可能导致的网络拥塞。这里还有一个小细节是对于同一个未发送队列（同一目标 IP 地址）可能会存在多个相同的数据报，这里的选择是放弃对这种情况的考虑，因为之前实现的 TCP 会帮我们处理掉这一切（回想一下 `Reassembler` 的工作就是实现不可靠字节流的重组）。
同样，在接收到数据包时，如果是 IP 数据包则直接发送到上层（推送到接收队列中）。如果是 ARP 请求，首先需要缓存对于发送主机的 ARP 映射（无论当前收到的是请求还是响应，白给的数据为什么不利用好呢？），然后在接收到请求并确认请求的目标 IP 地址是本机时，返回 ARP 响应，最后由于更新了 ARP 映射，因此此时可以将发送方主机 IP 下的未发送数据报全部发送。

Program Structure and Design of the NetworkInterface [Describe data
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
