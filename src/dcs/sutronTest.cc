#include "sutron.h"
// Main function
int
main ()
{
	std::string data0("\"B1$I/TLHG@A$@A$//////BF,BF,BF,BF,BF,BF,L(HL(H?GU?GS@0;@1^@1Y@03@1H@0F@2J@2O@0P@.#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@AH@A.@A\\@B-@BG@BY@A<@B8@CM@C-@C+@EA@AH@AB@@3@A)EV4E;XFR:E,&EA\\EO,C;/D0R@U @U!L");
	std::string data1("\"B1@@Gt@Gs@Sx@Sr@@i@@iEXTLBAAODAXe@R@A");
	std::string data2("\"C1+ABeHq@A@E|@FG@FM+BBeHq@A@@O.K"); // look for "C1+. the terminator . is important
//	std::string data2("\"C1+AA,T(@O @@ @@ @@ @@+BA,T(@O @@ @@ @@ @@+CA,T(@O @@ @@ @@ @@+DA,T(@O @@ @@ @@ @@+EA,T(@O @@ @@ @@ @@+FA,T(@O @@ @@ @@ @@+GA,T(@O @@ @@ @@ @@+HA,T(@O@T,@U6@U6@U6+IA,T(@O @@ @@ @@ @@.");
//	std::string data2("\"C1+AA,TY@O@PL@PL@PL@PL@PL@PL@PL@PL.J 2102527");
	std::string data3("\"D1D~A8@NI@NH@NG@NF@NE@DGF");
//	std::string data3("\"D1A,TT@GK@GK@GK@GK@GK@GK@GK@GL@GL@GK@GK@GK@@@@@@@@@@@@M");
	std::string data4("  :HG 1 #5 0.077 0.077 0.077 0.077 0.077 0.077 0.077 0.077 0.077 0.077 0.077 0.077 :VB 36 #60 13.6");
	std::string data5("\"MJ0.41 12.48 442.60 13.13MJ0.41 12.52 442.60 13.21"); 
	std::string data6("\":HG 7 #15 2.94 2.96 2.96 2.96 2.96 2.96 2.95 2.92 :BL 14.08");
	std::string data7("\"P85452401A1A>K@@^0T6V1AJ=@E@AFAH>AJZ4AM5BB6\"D<BL2@*.@E@\"@+G<BC -TV\"SAHAB@<@7@3@,");
	std::string buf;
	buf=sutron(data0);
	cout<<buf<<endl;

	return 0;
}
#include "sutron.cc"
