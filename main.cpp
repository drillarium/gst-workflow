#include <iostream>
#include "workflow.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

/* const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"pipe\" : \"fakesrc name='name1' delay='0' ! delay name='name2' delay='10' abort='5' error='3' ! fakesink name='name3'\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }"; */

// fakesrc -> delay -> fakesink
/* const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"pipe\" : \"fakesrc name='name1' delay='0' ! dummy_1 name='name2' ! fakesink name='name3'\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }"; */

const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"pipe\" : \"fakesrc name='name1' delay='0' ! dumm_app_worker_1 name='name2' ! fakesink name='name3'\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }";

// fakesrc -> delay -> fakesink -> terminator 
//         -> delay -> fakesink ->
/* const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"pipe\" : \"fakesrc name='name1' ! delay name='name2' ! fakesink name='name3' ! terminator name='name6' name1. ! delay name='name4' ! fakesink name='name5' ! name6.\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }"; */

// fakesrc -> delay -> fakesink
//         -> delay ->
/* const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"pipe\" : \"fakesrc name='name1' ! delay name='name2' ! fakesink name='name3' name1. ! delay name='name4' ! name3.\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }"; */

// fakesrc -> delay (ok)-> fakesink
//                  (error)-> fakesink
/* const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"pipe\" : \"fakesrc name='name1' ! delay name='name2' ! {ok} ! fakesink name='name3' name2. ! {error} ! fakesink name='name4'\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }"; */

// filesystemwatcher -> fakesink
/* const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"pipe\" : \"filesystemwatcher name='name1' ! delay name='name2' ! fakesink name='name3'\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }"; */

int main(int argc, char *argv[])
{
  // workflow
  workflow wf;

  // load workflow
  // if(argc == 1)
    wf.load(tmpl);
  /* else
    wf.load(argv[1]); */

  // start it. fakesrc generates jobs
  wf.start();

  // Wait
  int i = 0;
  while(true)
  {
    std::cin >> i;
    if(i == 0)
      break;
  }

  wf.stop();

  return 0;
}
