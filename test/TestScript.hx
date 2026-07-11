class TestCls
{
    public var a:Int = 1;
    public var b = 2.9;
    public var c:String = "Hello, World";

    public function new(a:Int, b:Float, c:String)
    {
        this.a = a;
        this.b = b;
        this.c = c;
    }

    public function printLog()
    {
        trace(a.toString() + ", " + b.toString() + ", " + "!");
        trace(this.c);
    }
}

var c = new TestCls(15, 30.75, "Test completed");
c.printLog();
