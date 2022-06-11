#include "SerialCmdLine.h"

SerialCmdLine::SerialCmdLine(Stream* stream_obj, cmd_def_t* user_cmd_list, bool local_echo, char* prompt, char* unknown_reply, bool higher_priori, uint32_t buffer_size)
{
    this->_buffer_size = buffer_size;
    this->_buffer = (uint8_t*)malloc(buffer_size);
    this->_buffer[0] = 0;
    this->_buff_idx = 0;
    this->_prev_char = 0;
    if (prompt != NULL) {
        int sl = strlen(prompt);
        this->_prompt = (char*)malloc(sl + 1);
        strcpy(this->_prompt, prompt);
    }
    else {
        this->_prompt = NULL;
    }
    if (unknown_reply != NULL) {
        int sl = strlen(unknown_reply);
        this->_unknown_reply = (char*)malloc(sl + 1);
        strcpy(this->_unknown_reply, unknown_reply);
    }
    else {
        this->_unknown_reply = NULL;
    }
    this->_stream = stream_obj;
    this->_echo = local_echo;
    this->_cmd_list = user_cmd_list;
    this->_higher_priori = higher_priori;
}

int SerialCmdLine::task()
{
    int ret = 0;
    if (_stream->available() <= 0) { // nothing to do
        return ret;
    }
    do
    {
        uint8_t c = _stream->read();
        ret += 1;
        if (_echo) {
            _stream->write((char)c);
        }
        if (c == '\n' && this->_prev_char == '\r')
        {
            this->_prev_char = c;
            return ret;
        }
        if (c == '\0' || c == '\r' || c == '\n')
        {
            _buffer[_buff_idx] = '\0';
            this->_stream->write("\r\n");
            if (_buff_idx == 0) {
                if (this->_prompt != NULL) {
                    this->_stream->write(this->_prompt, strlen(this->_prompt));
                }
                this->_prev_char = c;
                return ret;
            }

            uint8_t ti;
            for (ti = 0; ; ti++)
            {
                cmd_def_t* cmd = &(_cmd_list[ti]);
                if (cmd->cmd_header == NULL || cmd->cmd_header[0] == '\0' || cmd->handler_callback == NULL) {
                    break; // end of table reached
                }
                uint8_t sl = strlen(cmd->cmd_header);
                char endchar = _buffer[sl];
                if (memcmp(cmd->cmd_header, _buffer, sl) == 0 && (endchar == '\0' || endchar == ' ' || endchar == '\t' || endchar == '\r' || endchar == '\n')) {
                    if (endchar == ' ' || endchar == '\t') {
                        sl += 1;
                    }
                    char* ptr = (char*)&(_buffer[sl]);
                    cmd->handler_callback(cmd, ptr, _stream);
                    if (this->_prompt != NULL) {
                        this->_stream->write(this->_prompt, strlen(this->_prompt));
                    }
                    this->_prev_char = c;
                    _buff_idx = 0;
                    _buffer[_buff_idx] = 0;
                    return ret * -1;
                }
            }

            if (this->_unknown_reply != NULL) {
                if (_echo) {
                    this->_stream->write("\r\n");
                }
                this->_stream->write(this->_unknown_reply, strlen(this->_unknown_reply));
            }
            this->_stream->write("\r\n");
            this->_stream->write(this->_prompt, strlen(this->_prompt));
            this->_prev_char = c;
            _buff_idx = 0;
            _buffer[_buff_idx] = 0;
        }
        else {
            if (_buff_idx > 0 && c == 0x08 && _echo) { // backspace
                _buff_idx -= 1;
                _buffer[_buff_idx] = 0;
                this->_prev_char = c;
            }
            else if ((_buff_idx + 1) < _buffer_size) {
                _buffer[_buff_idx] = c;
                _buff_idx += 1;
                _buffer[_buff_idx] = 0;
                this->_prev_char = c;
            }
        }
    }
    while (_stream->available() > 0 && _higher_priori);
    return ret;
}

void SerialCmdLine::print_prompt() {
    if (this->_prompt != NULL) {
        this->_stream->write(this->_prompt, strlen(this->_prompt));
    }
}
