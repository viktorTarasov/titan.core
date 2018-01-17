# generates html code from a testresult xml. The testresults.css can be used to format the result
cat $1 | awk -f testresultsXML2HTML.awk
