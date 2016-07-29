#ifndef UTILITIES_H
#define UTILITIES_H

inline bool buf2int(const char* buffer, unsigned int length, int& result)
{
    if (buffer==nullptr || length<=0) return false;
    int flag = 1;
    if (buffer[0]=='-' || buffer[0]=='+')
    {
        --length;
        ++buffer;
        flag = buffer[0]=='-'?-1:1;
    }
    if (length == 0)
        return false;
    result = 0;
    for (int i=0; i<length; ++i) {
        if (isdigit(buffer[i]))
            result = result*10 + buffer[i]-'0';
        else
            return false;
    }
    result *= flag;
    return true;
}

inline int current_unix_seconds()
{
    return time(nullptr);
}
#endif // UTILITIES_H
