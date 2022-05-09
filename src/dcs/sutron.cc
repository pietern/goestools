#include "sutron.h"

std::vector<std::string> split_delim(const std::string &s, char delim)
{
	std::stringstream ss(s);
	std::string item;
	std::vector<std::string> elems;
	while (std::getline(ss, item, delim))
	{
	//	elems.push_back(item);
	        elems.push_back(std::move(item)); // if C++11 (based on comment from @mchiasson)
	}
	return elems;
}

constexpr size_t Hash(const char* str)
{
	const long long p = 131;
	const long long m = 4294967291; // 2^32 - 5, largest 32 bit prime
	long long total = 0;
	long long current_multiplier = 1;
	for (int i = 0; str[i] != '\0'; ++i)
	{
        	total = (total + current_multiplier * str[i]) % m;
        	current_multiplier = (current_multiplier * p) % m;
	}
	return total;
}

std::string	sutron(std::string inbuf)
{	
	std::string outbuf;
	std::string key1(":YB ");
	std::string key2(":BL ");
	std::string key3(" ");
	char coda[40];
	int battery;
	std::size_t foundYB=inbuf.find(key1), foundBL=inbuf.find(key2), foundSP=inbuf.rfind(key3), len;
//	battery is set to 1, to display bat: xx.xx Voltages.  Else "0" don't show them, or are not present.
	battery=1;
	coda[0]=' ';
	coda[1]='\0';
				 // cout<<inbuf<<endl;
/*
	Hacks for find, if statements, used to set the last line of actual data.
	The rest of the data is optional.  I set opt: in the output for these.
*/
	if (foundYB!=std::string::npos)
//		find a YB battery voltage and set length to not pass this optional data.
	{
		len=foundYB-1;
		std::size_t length = inbuf.copy(coda, sizeof(coda), foundYB);
		coda[length]='\0';
		battery=0;
	}
	else
		len=inbuf.length()-1;

	if (foundBL!=std::string::npos)
//		find a BL battery voltage and set length to not pass this optional data.
	{
		len=foundBL-1;
		std::size_t length = inbuf.copy(coda,sizeof(coda), foundBL);
		coda[length]='\0';
		battery=0;
	}
	else
		len=inbuf.length()-1;

	if (inbuf[inbuf.size()-1] == ' ' && (battery == 1) )
/*
		find a Space last character. Optional are space delimited. Find the
		next space in reverse, then set length to not pass this optional data.
		The char variable coda, contains the space delimited value.
*/
	{
		inbuf[inbuf.size()-1]='\0';
		foundSP=inbuf.rfind(key3);
		len=foundSP-1;
		if ( foundSP == 0 || foundSP == std::string::npos)
			len=inbuf.length()-1;
		else
		{
			std::size_t length = inbuf.copy(coda,sizeof(coda), foundSP+1);
			coda[length]='\0';
		}
	}

	if ( inbuf.length() < 3)
			return inbuf;		

	switch ( Hash( inbuf.substr(1,2).c_str() ) )
	{
		case Hash("B\\"):	
		case Hash("B*"):	
		case Hash("B1"):	
		case Hash("B2"):	
		case Hash("B3"):	
		case Hash("B4"):	
		case Hash("BC"):	
		case Hash("BG"):	
		case Hash("BK"):	
		case Hash("BL"):	
		case Hash("BP"):	
		case Hash("BS"):	
		case Hash("BX"):	
		case Hash("BY"):	
				{
				  std::string tmp("");
				  tmp="block-id: B ";
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));

				  long int ival,l,m,r;
				  float bat;

				  std::string c(1,inbuf[2]); // convert 1 char group-id to string
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
				  bat=0;
				  bat=((inbuf[len]-64)&0x3F)*.234 + 10.6;
				  if ( battery == 1)
				
				  {
					std::string s;
					s=coda;
				 	//tmp="opt: " + s + " bat: " + to_string(bat) + " Volts\n";
				 	tmp="opt: " + s + " bat: " + to_string( round(bat * 10)/10).substr(0,4)  + " Volts\n";
				    	std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  }
				  else
				  {
					std::string s;
					s=coda;
				 	tmp="opt: " + s + "\n";
				    	std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  }

				}
			  	break;

		case Hash("C1"): 	
	//	case Hash("C2"): 	
	//	case Hash("C3"): 
	//	case Hash("C4"):
				{
				  std::vector<std::string> word = split_delim(inbuf, '.');
				
				  std::string tmp("");
				  std::string tmp2("");
				  float bat;
			 	  long int ival,l,m,r;
				  int	lenbuf=inbuf.length();
				  len=word[0].length()+1;

				  if (lenbuf -len  > 0 )
				  {
                                        std::string s;
                                        s=coda;
					bat=0;
                                        bat= ((inbuf[len] -64)&0x3F)*.234 + 10.6;
                                        tmp="opt: " + s + " bat: " + to_string( round(bat * 10)/10).substr(0,4)  + " Volts\n";
                                        std::copy(tmp.begin(), tmp.end(), std::back_inserter(tmp2));
				  }
				  std::vector<std::string> pword = split_delim(word[0], '+');
				  if ( pword[0].empty()) 
					return inbuf;  // see if it is really a C type, as it is required to have +

				  std::string c(1,inbuf[2]); // convert 1 char group-id to string
				  tmp="block-id: C group-id: " + c + " "; 
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  std::string sensor;
				  std::string smeas, sj_day, stmin_in_day, stmin_int;
				  long s,jday,tday,tint;
				  //int j=1; // skip the "C1: piece
				  for (std::size_t j=1; j < pword.size(); j++)
				 {
				   std::string c(1,inbuf[2]); // convert 1 char group-id to string
				   std::string data("");
				   sensor=pword[j];
				   len=sensor.length();

				   for(std::size_t i=7;i<len;i=i+3)
                                   {
                                    	if (i+2 > len)
                                        	r=0;
                                    	else
					{
                                        	r= (sensor[i+2] - 64)&0x3F; // right of 18 bit percision
					}

                                    	if (i+1 > len)
                                       		m=0 <<6;
                                    	else
					{
                                        	m= ((sensor[i+1] - 64)&0x3F) <<6; // middle of 18 bit percision
					}

					if (i > len)
						l=0;
					else
					{
                                    		l= ((sensor[i] - 64)&0x3F) <<12; // left of 18 bit percision
					}

                                    	ival= l + m + r;
                                    	tmp=to_string(ival) + " ";
                                    	std::copy(tmp.begin(), tmp.end(), std::back_inserter(data));
                                     }

					s=(sensor[0]-64)&0x3F;
					smeas=to_string(s);

					l=((sensor[1]-64)&0x3F)<<6 ;
					r=(sensor[2]-64)&0x3F;
					jday=l+r;
					sj_day=to_string(jday);

					l=((sensor[3]-64)&0x3F)<<6 ;
					r=(sensor[4]-64)&0x3F;
					tday=l+r;
					stmin_in_day=to_string(tday);

					l=((sensor[5]-64)&0x3F)<<6 ;
					r=(sensor[6]-64)&0x3F;
					tint=l+r;
					stmin_int=to_string(tint);

				  	tmp=" M" + smeas + ": " + "Julian Day: " + sj_day
					+ " Time Since Midnight(min): " + stmin_in_day + " Interval(min): "
					+ stmin_int + " data(x10^-2): " + data + " ";
				  	std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  } 

				  std::copy(tmp2.begin(), tmp2.end(), std::back_inserter(outbuf));
				}
				
			  	break;

		case Hash("D1"): 	
		case Hash("D2"): 	
		case Hash("D3"): 	
		case Hash("D4"): 	
				{
				  std::string tmp("");
				  tmp="block-id: D ";
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));

				  long int ival,l,m,r;
				  float bat;

				  std::string c(1,inbuf[2]); // convert 1 char to string
				  tmp="group-id: " + c + " ";
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));

// ------------------->		    put in the jday and time mins <---------------
   				  r= (inbuf[4] - 64)&0x3F;       // low byte jday
				  l= ((inbuf[3] - 64)&0x3F) <<6; // high byte jday
				  ival=l+r;
				  tmp=" Julian Day: " + to_string(ival) + " ";
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));

   				  r= (inbuf[6] - 64)&0x3F;       // low byte mins from midnight
				  l= ((inbuf[5] - 64)&0x3F) <<6; // high byte mins from midnight
				  ival=l+r;
				  tmp=" Time Since Midnight(mins): " + to_string(ival) + " ";
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  tmp=" data(x10^-2): ";
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));

// ---------------------------------
				  for(std::size_t i=7;i<len;i=i+3)
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
				  bat=0;
				  bat=((inbuf[len]-64)&0x3F)*.234 + 10.6;
				  if ( battery == 1)
				
				  {
					std::string s;
					s=coda;
				 	//tmp="opt: " + s + " bat: " + to_string(bat) + " Volts\n";
				 	tmp="opt: " + s + " bat: " + to_string( round(bat * 10)/10).substr(0,4)  + " Volts\n";
				    	std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  }
				  else
				  {
					std::string s;
					s=coda;
				 	tmp="opt: " + s + "\n";
				    	std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				  }

				}
			  	break;

		case Hash(" :"): 	
				{
				  std::string tmp("");

				  tmp="block-id: SHEF\n"+ inbuf;
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				}
			  	break;

		case Hash("MJ"):
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
				  tmp=inbuf;
				  std::copy(tmp.begin(), tmp.end(), std::back_inserter(outbuf));
				}
	} // end switch()

	return outbuf;
}

