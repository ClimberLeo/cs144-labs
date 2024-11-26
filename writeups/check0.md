Checkpoint 0 Writeup
====================

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to credit/thank these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

My secret code from section 2.1 was: [code here]

I was surprised by or edified to learn that: [describe]

Describe ByteStream implementation. [本实验实现了一个基于内存的字节流，作为热身实验，相对而言比较简单，无论是思路还是实现上。唯一需要注意的是性能方面，对于字节流这种在底层计算机系统中对性能极其敏感的部分，数据结构的选用可能会对性能产生巨大的影响，起初我直接使用了string作为底层缓冲区存储数据，实测性能只有2Gbit/s左右；后来又尝试了queue+string的实现方式，传输速率达到了19.10Gbit/s，提升了将近十倍。究其原因，首先字节流FIFO的特性和队列天然地吻合，其次由于我又使用了string_view的remove_prefix()方法来模拟对string的部分pop，这使得底层的字符串一旦放入队列直到整个被pop出队列前，就不会再被修改，因此最大的开销得以被避免，读和写都变成了O(1)（计算机体系结构中的一个伟大思想是“使常见情况更快”，而字节流在底层系统中有着巨大的使用量，因此对其做出性能优化是必要且有价值的）。]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I'm not sure about: [describe]

- Optional: I contributed a new test case that catches a plausible bug
  not otherwise caught: [provide Pull Request URL]