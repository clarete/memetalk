meme foo
requires test, io, qt
where
 io = central:stdlib/io
 qt = central:stdlib/qt



f: fun(x) {
   return 10;
}

main: fun() {
  set_debugger_module(thisModule);
  dbg();
  var a = 1 + 2;
  f(a + a + a);
  return a;
}

class Tester
fields: process;
init new: fun(proc) {
  @process = proc;
}
instance_method step0: fun() {
  test.assertLocation(@process, "test_debugger_step_line1/main", [15, 14, 15, 15]);
  @process.stepOverLine(); // '2' POP dbg() result
}

instance_method step1: fun() {
  // var a = 1 + {2}
  test.assertLocation(@process, "test_debugger_step_line1/main", [15, 14, 15, 15]);
  @process.stepOverLine();
}

instance_method step2: fun() {
  // f(a + a + {a});
  test.assertLocation(@process, "test_debugger_step_line1/main", [16, 12, 16, 13]);
  @process.stepOverLine();
}

instance_method step3: fun() {
  //return {a};
  test.assertLocation(@process, "test_debugger_step_line1/main", [17, 9, 17, 10]);
  @process.detach_debugger();
}
end

debug: fun(proc) {
  var app = qt.QApplication.new(); //in case target process didn't started it
  test.set_debugger_tests(Tester.new(proc), 4);
  return test;
}