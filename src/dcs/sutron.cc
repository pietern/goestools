#include "sutron.h"

std::string	sutron(std::string inbuf)
{	
	std::string outbuf;
	std::string key1(":YB ");
	std::string key2(":BL ");
	std::string key3(" ");
	char coda[40];
	int battery;
	std::size_t found1=inbuf.find(key1), found2=inbuf.find(key2), found3=inbuf.rfind(key3), len;
	battery=1;
	coda[0]=' ';
	coda[1]='\0';
				  //cout<<inbuf<<endl;

	if (found1!=std::string::npos)
	{
		len=found1-1;
		std::size_t length = inbuf.copy(coda,sizeof(coda), found1);
		coda[length]='\0';
		battery=0;
	}
	else
		len=inbuf.length()-1;

	if (found2!=std::string::npos)
	{
		len=found2-1;
		std::size_t length = inbuf.copy(coda,sizeof(coda), found2);
		coda[length]='\0';
		battery=0;
	}
	else
		len=inbuf.length()-1;

	if (inbuf[inbuf.size()-1] == ' ' && (battery == 1) )
	{
		inbuf[inbuf.size()-1]='\0';
		found3=inbuf.rfind(key3);
		len=found3-1;
		if ( found3 == 0 || found3 == std::string::npos)
			len=inbuf.length()-1;
		else
		{
			std::size_t length = inbuf.copy(coda,sizeof(coda), found3+1);
			coda[length]='\0';
		}
	}

	switch ( inbuf[1] )
	{
		case 'B':	
				{
				  std::string tmp("");
				  tmp="block-id: B ";
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));

				  long int ival,l,m,r;
				  float bat;

				  std::string c(1,inbuf[2]); // convert 1 char to string
				  tmp="group-id: " + c + " ";
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));

				  tmp="time offset(min): " + to_string( (inbuf[3]-64)&0x3F ) + " data(x10^-2): ";
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  for(std::size_t i=4;i<len;i=i+3)
				  {
				    if (i+2 > len) 
					r=0;
				    else	
					r= (inbuf[i+2] - 64)&0x3F; // right of 18 bit percision
					
				    if (i+1 > len) 
					m=0 <<6;
				    else
				    	m= ((inbuf[i+1] - 64)&0x3F) <<6; // middle of 18 bit percision

			  	    l= ((inbuf[i] - 64)&0x3F) <<12; // left of 18 bit percision
				    ival= l + m + r;
				    tmp=to_string(ival) + " ";
				    std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  }
				  if ( inbuf[len] < 64)
					battery=0;
				  bat=((inbuf[len]-64)&0x3F)*.234 + 10.6;
				  if ( battery == 1)
				
				  {
					std::string s;
					s=coda;
				 	//tmp="opt: " + s + " bat: " + to_string(bat) + " Volts\n";
				 	tmp="opt: " + s + " bat: " + to_string( round(bat * 100)/100).substr(0,5)  + " Volts\n";
				    	std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  }
				  else
				  {
					std::string s;
					s=coda;
				 	tmp="opt: " + s + "\n";
				    	std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  }

				  //cout<<outbuf<<endl;
				}
			  	break;

		case 'C': 	
				{
				  std::string tmp("");

				  tmp="block-id: C\n"+ inbuf;
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				}
				
			  	break;
		case 'D': 	
				{
				  std::string tmp("");

				  tmp="block-id: D\n"+ inbuf;
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				}
			  	break;
		case ' ': 	
				{
				  std::string tmp("");

				  tmp="block-id: SHEF\n"+ inbuf;
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				}
			  	break;
		case ':': 	
				{
				  std::string tmp("");

				  tmp="block-id: SHEF\n"+ inbuf;
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				}
			  	break;
		case 'M':
				{
				  std::string tmp("");

				  inbuf=std::regex_replace(inbuf,std::regex("MJ"), "\r\n");
				  tmp="block-id: ASCII\n" + inbuf;
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				}
			  	break;
		default:  	
				{
				  std::string tmp("");

				  tmp="block-id: UNKNOWN\n"+ inbuf;
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				}
	} // end switch()

	return outbuf;
}
