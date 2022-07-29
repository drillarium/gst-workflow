#include <iostream>
#include "workflow.h"

// fakesrc -> delay -> fakesink
const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"template\" : \"fakesrc name='name1' ! delay name='name2' ! delay name='name4' ! fakesink name='name3'\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }";

// fakesrc -> delay -> fakesink -> terminator 
//         -> delay -> fakesink ->
/* const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"template\" : \"fakesrc name='name1' ! delay name='name2' ! fakesink name='name3' ! terminator name='name6' name1. ! delay name='name4' ! fakesink name='name5' ! name6.\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }"; */

// fakesrc -> delay -> fakesink
//         -> delay ->
/* const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"template\" : \"fakesrc name='name1' ! delay name='name2' ! fakesink name='name3' name1. ! delay name='name4' ! name3.\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }"; */

// fakesrc -> delay (ok)-> fakesink
//                  (error)-> fakesink
/* const char *tmpl = "{\
                     \"type\": \"workflow\",\
                     \"template\" : \"fakesrc name='name1' ! delay name='name2' ! {ok} ! fakesink name='name3' name2. ! {error} ! fakesink name='name4'\",\
                     \"uid\" : \"workflow uid\",\
                     \"name\" : \"workflow name\"\
                    }"; */
int main()
{
  workflow wf;

  wf.load(tmpl);

  wf.start();

  // Wait
  int i = 0;
  while(true)
  {
    std::cout << "0 to quit, 1 to abort job";
    std::cin >> i;
    if(i == 0)
      break;
    if(i == 1)
      wf.removeJob("0");
  }

  wf.stop();

  return 0;
}
