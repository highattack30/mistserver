#include <map>
#include <stdlib.h>

class HTTPReader{
  public:
    HTTPReader();
    bool ReadSocket(int CONN_fd);
    std::string GetHeader(std::string i);
    std::string GetVar(std::string i);
    void SetHeader(std::string i, std::string v);
    void SetHeader(std::string i, int v);
    void SetVar(std::string i, std::string v);
    void SetBody(std::string s);
    void SetBody(char * buffer, int len);
    std::string BuildRequest();
    std::string BuildResponse(std::string code, std::string message);
    void Clean();
    std::string method;
    std::string url;
    std::string protocol;
    unsigned int length;
  private:
    bool seenHeaders;
    bool seenReq;
    bool parse();
    std::string HTTPbuffer;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> vars;
    void Trim(std::string & s);
};//HTTPReader

HTTPReader::HTTPReader(){Clean();}
void HTTPReader::Clean(){
  seenHeaders = false;
  seenReq = false;
  method = "GET";
  url = "/";
  protocol = "HTTP/1.0";
  length = 0;
  HTTPbuffer = "";
  headers.erase(headers.begin(), headers.end());
  vars.erase(vars.begin(), vars.end());
}

std::string HTTPReader::BuildRequest(){
  std::map<std::string, std::string>::iterator it;
  std::string tmp = method+" "+url+" "+protocol+"\n";
  for (it=headers.begin(); it != headers.end(); it++){
    tmp += (*it).first + ": " + (*it).second + "\n";
  }
  tmp += "\n";
  tmp += HTTPbuffer;
  return tmp;
}

std::string HTTPReader::BuildResponse(std::string code, std::string message){
  std::map<std::string, std::string>::iterator it;
  std::string tmp = protocol+" "+code+" "+message+"\n";
  for (it=headers.begin(); it != headers.end(); it++){
    tmp += (*it).first + ": " + (*it).second + "\n";
  }
  tmp += "\n";
  tmp += HTTPbuffer;
  return tmp;
}

void HTTPReader::Trim(std::string & s){
  size_t startpos = s.find_first_not_of(" \t");
  size_t endpos = s.find_last_not_of(" \t");
  if ((std::string::npos == startpos) || (std::string::npos == endpos)){s = "";}else{s = s.substr(startpos, endpos-startpos+1);}
}

void HTTPReader::SetBody(std::string s){
  HTTPbuffer = s;
  SetHeader("Content-Length", s.length());
}

void HTTPReader::SetBody(char * buffer, int len){
  HTTPbuffer = "";
  HTTPbuffer.append(buffer, len);
  SetHeader("Content-Length", len);
}


std::string HTTPReader::GetHeader(std::string i){return headers[i];}
std::string HTTPReader::GetVar(std::string i){return vars[i];}

void HTTPReader::SetHeader(std::string i, std::string v){
  Trim(i);
  Trim(v);
  headers[i] = v;
}

void HTTPReader::SetHeader(std::string i, int v){
  Trim(i);
  char val[128];
  sprintf(val, "%i", v);
  headers[i] = val;
}

void HTTPReader::SetVar(std::string i, std::string v){
  Trim(i);
  Trim(v);
  vars[i] = v;
}

bool HTTPReader::ReadSocket(int CONN_fd){
  //returned true als hele http packet gelezen is
  int r = 0;
  int b = 0;
  char buffer[500];
  while (true){
    r = DDV_ready(CONN_fd);
    if (r < 1){
      if (r == 0){
        socketError = true;
        #if DEBUG >= 1
        fprintf(stderr, "User socket is disconnected.\n");
        #endif
      }
      return parse();
    }
    b = DDV_iread(buffer, 500, CONN_fd);
    HTTPbuffer.append(buffer, b);
  }
  return false;
}//HTTPReader::ReadSocket


bool HTTPReader::parse(){
  size_t f;
  std::string tmpA, tmpB, tmpC;
  while (HTTPbuffer != ""){
    if (!seenHeaders){
      f = HTTPbuffer.find('\n');
      if (f == std::string::npos) return false;
      tmpA = HTTPbuffer.substr(0, f);
      HTTPbuffer.erase(0, f+1);
      if (!seenReq){
        seenReq = true;
        f = tmpA.find(' ');
        if (f != std::string::npos){method = tmpA.substr(0, f); tmpA.erase(0, f+1);}
        f = tmpA.find(' ');
        if (f != std::string::npos){url = tmpA.substr(0, f); tmpA.erase(0, f+1);}
        f = tmpA.find(' ');
        if (f != std::string::npos){protocol = tmpA.substr(0, f); tmpA.erase(0, f+1);}
        //TODO: GET variable parsing
      }else{
        if (tmpA[0] == '\n'){
          seenHeaders = true;
          if (GetHeader("Content-Length") != ""){length = atoi(GetHeader("Content-Length").c_str());}
        }else{
          f = tmpA.find(':');
          if (f == std::string::npos) continue;
          tmpB = tmpA.substr(0, f);
          tmpC = tmpA.substr(f+1);
          SetHeader(tmpB, tmpC);
        }
      }
    }
    if (seenHeaders){
      if (length > 0){
        //TODO: POST variable parsing
        return (HTTPbuffer.length() >= length);
      }else{
        return true;
      }
    }
  }
  return false; //we should never get here...
}//HTTPReader::parse

