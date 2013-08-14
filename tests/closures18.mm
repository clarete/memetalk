module foo() {
  main: fun() {
    var cmod = get_compiled_module(thisModule);

    var cfn = CompiledFunction.newTopLevel(
      "bar", "fun(a) { return a + 1; }", cmod);

    cfn.setCode("fun(a,b) { return a + b; }");

    var fn = cfn.instantiate(thisModule);

    assert(fn(2,3) == 5, "Changing code of toplevel function");
  }
}