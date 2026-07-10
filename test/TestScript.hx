class TestCls
{
    public var a:Int = 1;
    public var b:Float = 2;
    public var c:String = "你好，脚本！";

    public function new(a:Int,b:Float,c:String)
    {
        this.a = a;
        this.b = b;
        this.c = c;
    }

    public function printLog()
    {
        trace(a.toString() + b.toString() + "！");
        trace(this.c);
    }
}

var c = new TestCls(1,2,"Hello!");
c.printLog();

var beforeTime = now();

var loopTime = 0;
while(loopTime <= 100000)
{
    loopTime = loopTime + 1;
}

var afterTime = now();

trace("执行100000次循环，耗时" + (afterTime - beforeTime) + "秒");