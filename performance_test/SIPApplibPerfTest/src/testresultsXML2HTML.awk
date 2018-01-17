BEGIN{
  FS="[<>:]"
}
/:name/{
  print "<h2>Test Results of " $4"</h2>"
}
/purpose/{
  print "<h3>Purpose: "$4 "</h3>\n<table class=\"ExecutionTime\">"
}
/start_time/{
  print "<tbody><tr><td>Start Time:</td><td>" $4 "</td></tr>"
}
/end_time/{
  print "<tr><td>End Time:</td><td>" $4 "</td></tr></tbody></table>"
}
/<tq[0-9]+:environment/{
  print "<div class=\"EnvironmentTable\"><p class=\"tableTitle\">Environment settings</p><table><tbody><tr class=\"Header\"><th>Environment setting</th><th>Value</th></tr>";
  env=1;
}
/<\/tq[0-9]+:environment/{
  env=0;
}
//{
  if(env==1){
    print "<tr class=\"EnvItems\"><td>"$3"</td><td>"$4 "</td></tr>"
  }
}
/<tq[0-9]+:parameter/{
  if (par==0) {
    print "</tbody></table></div><div class=\"ParamTable\"><p class=\"tableTitle\">Parameters</p><table><tbody><tr class=\"Header\"><th>Parameter</th><th>Value [Unit]</th></tr>";
  }
  par=1;
  split($0,tmp,"[<>=']");
  print "<tr class=\"Params\"><td>"tmp[4]"</td><td>"tmp[9]" ["tmp[7] "]</td></tr>"
}
/:result/{
  if (res==0) {
    print "</tbody></table></div><div class=\"ResultTable\"><p class=\"tableTitle\">Test Results</p><table><tbody><tr class=\"Header\"><th>Result</th><th>Value [Unit]</th></tr>";
  }
  res=1;
  split($0,tmp,"[<>=']");
  print "<tr class=\"ResItems\"><td>"tmp[4]"</td><td>"tmp[9]" ["tmp[7] "]</td></tr>"
}
/<\/tq[0-9]+:testresult>/{
  print "</tbody></table></div>"
  par=0;
  res=0;
}
