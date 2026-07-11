class ScriptClass extends TestClass
{
    public override function c(val)
    {
        return super.c(val) + 5;
    }
}

var cls = new ScriptClass();
trace(cls.c(10));