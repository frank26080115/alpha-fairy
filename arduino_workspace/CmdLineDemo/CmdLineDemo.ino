#include <SerialCmdLine.h>

void test_func(void* cmd, char* argstr, Stream* stream);
void echo_func(void* cmd, char* argstr, Stream* stream);

cmd_def_t cmds[] = {
  { "test", test_func },
  { "echo", echo_func },
  { NULL, NULL }, // end of table
};

SerialCmdLine cmdline(&Serial, (cmd_def_t*)cmds, false, (char*)">>>", (char*)"???", true, 512);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  cmdline.print_prompt();
}

void loop() {
  // put your main code here, to run repeatedly:
  cmdline.task();
}

void test_func(void* cmd, char* argstr, Stream* stream)
{
  stream->println("test");
}

void echo_func(void* cmd, char* argstr, Stream* stream)
{
  stream->println(argstr);
}
