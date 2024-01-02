# 实验二

对于GBN和SR协议的分别实现。

笔者的代码是基于知乎上某个简单的收发服务器修改而成，如果你想要自己实现本实验，可以参考笔者的出发思路。

笔者不能保证本实验的代码是完全正确的。笔者在SR的测试中发现了不知名的确认问题，修改`SRServer:106`之后的部分代码来产生大量的packet以掩盖该问题。如果你发现了错误的原因，欢迎提出issue。

除此之外，全双工需要你实现可以同时收发的功能，而笔者的thread不能正常运行，全双工功能并未完全实现，此处采用了在主函数中修改调用函数的方式来实现全双工。