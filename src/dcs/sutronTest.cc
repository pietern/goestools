#include "sutron.h"
// Main function
int
main ()
{
//	std::string data0(" B1@@Gt@Gs@Sx@Sr@@i@@iL");
//	std::string data0("\"BN^D<1@XXF5HBW BN_D< @XVF% B(?BNVD;9@XUF((B*\\BN\\D;Y@XRGI\\B);BN^D;Q@XPG\\LB&9BN#D:9@XMG\\LB$OBN%D:Q@XJGZ(B\"KBN?D95@XEG(,B %CV DO\\DTICVSDO[DTP");
//	std::string data0(" B\\A@AO@C?@DB@NX@NN@@I@@N@@#@^(@],@^@@@^@@^@@@@@@@A9@@JE");
//	std::string data0(" B[#@A\"@DD@DG@O@@N6@@B@@B@@G@7(@I,@[4@@C@@K@@@@@@@B@@@JH");
//	std::string data0(" B6E#@L@1DNI;@.(@@CAU,BLDK@H@'DPI0@.(@@CAU,BBD-@N@");
	std::string data0(" B*B@KX@KW@KY@KW@'&@&'@']@',@A<@A>@A?@A?@@A@@A@@A@@A@I.@I1@I1@I1@BBH");
 //	std::string data0("\"B1$I/TLHG@A$@A$//////BF,BF,BF,BF,BF,BF,L(HL(H?GU?GS@0;@1^@1Y@03@1H@0F@2J@2O@0P@.#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@AH@A.@A\\@B-@BG@BY@A<@B8@CM@C-@C+@EA@AH@AB@@3@A)EV4E;XFR:E,&EA\\EO,C;/D0R@U @U!L");
//	std::string data1("\"B1@@Gt@Gs@Sx@Sr@@i@@iL");
//	std::string data1("\"B2@@Gt@Gs@Sx@Sr@@i@@iL");
	std::string data1("\"B1O@!&@!&@!'@!(@!'@!'@!'@!&@!&@!(@!&@!%@!&@!'@!&@!'JAH@@");
	std::string data2("\"C1+AA9L[@O@PA@PA@PA@PA@PA@P@@P@@P@.H 2102527 ");
//	std::string data2("\"C1+ABeHq@A@E|@FG@FM+BBeHq@A@@O.K"); // look for "C1+. the terminator . is important
//	std::string data2("\"C1+AA,T(@O @@ @@ @@ @@+BA,T(@O @@ @@ @@ @@+CA,T(@O @@ @@ @@ @@+DA,T(@O @@ @@ @@ @@+EA,T(@O @@ @@ @@ @@+FA,T(@O @@ @@ @@ @@+GA,T(@O @@ @@ @@ @@+HA,T(@O@T,@U6@U6@U6+IA,T(@O @@ @@ @@ @@.");
//	std::string data2("\"C1+AA,TY@O@PL@PL@PL@PL@PL@PL@PL@PL.J 2102527");
//	std::string data2("\"C1+AA6O^@O@O?@O?@O?@O?@P@@O?@O?@O?.I 2102527");
//	std::string data2("\"C1+AA9U$@O@PB@PB@PB@PB@PB@PB@PB@PB.J 2102527 ");
//	std::string data2(" C1+AA;BG@O@PA@PA@PA@PA@PA@PA@PA@PB.I2102527 ");
//	std::string data2(" C3D*@YAZD^G\"@.:@@@AVWD3D=@OAGDXG=@.:@@@AVWCRDV@ZARDWG1@.:@@@AVWCYE@@TAPDOHT@.:@@@AVWCQE @\"AMDPH^@.:@@@AVWD/C4@M@5DPH$@.:@@@AVXDOD0@PAKDMH-@.:@@@AVXDGD*@M@2DGIR@.9@@@AVXC E$@YATDAI%@.:@@@AVYCTD]@\\A9C;I1@.:@@@AVYB>D/@TA\"C8JU@.:@@@AVYA=AC@XAHC5J*@.:@@@AVYBI");
//	std::string data2(" C4ES@R@&CRL/@.2@@[??PC+D&@\\@+CSL.@.2@@[??JC\"D@@^@3CTL-@.2@@[??FC&D'@_@4CTL+@.2@@[??AC'EH@\\@6CUL)@.2@@[?>;C)DP@^@9CVL(@.2@@[?>6C+D0@#@<CWL(@.3@@[?>1C)DI@(AKCWL&@.2@@[?>,C(D @2ARCXL&@.2@@[?>)C,D_@1A\\CXL$@.3@@[?>'C%DN@*ACCYL\"@.3@@[?>$C*DJ@^AFCZL^@.3@@[?>!A?");
//	std::string data2("\"CS#!0101XX-@PM1457N&_&@D@\\@P@R@#@B@B@A@A-@0FPBVD,D+-(5?.F");
//	std::string data2("\"CS#!0101XX-@PM1457N&_&@D@\\@P@R@#@B@B@A@A-@0FPBVD,D+-(5?.@V@NNHYU@B@@@@@@@@ANC(M-@@@@@@@@AQB1ARB1@L@L@@@@BSC?@@A9+A@QZ@@W@U@IA;BG+B@TJ@@@@S@IBRBS+C@U9@@@@T@IBLBL+D@XK@@\"@V@JBFBF+E@W3@@=@V@JBBBB+F@V^@A!@X@KA>A>+G@ZG@A[@X@JA:A7+H@Z9@A0@W@JA5A/+I@[9@BP@W@KA1A)+J@];@BO@X@JA.A$+K@^2@BU@V@IA+A +L@^*@BI@U@JA+A^+M@^5@A6@Y@JA+A]+N@_7@BA@W@JA)A[+O@ Y@A=@T@IA&AZ+P@^=@A2@V@KA$AW+Q@]0@@[@[@NA#AU+R@\\E??O@ @NA#AV+S@X:?=6@ @NA%AX+T@XE?=U@_@OA%AY+U@T7?;.@ @MA(AX+V@Q<?:3@Y@KA,AR+W@P+?:<@X@JA+AP+X@NR?;^@Y@LA+AO+Y@LT?<I@Z@MA*AN+Z@JF?<A@X@LA,AM+[@H-?<!@[@KA'AM+\\@I.?;5@X@KA-AJ+]@H3?;?@X@KA-AJ+^@F[?=O@W@KA*AJ+_@F=?<2@W@KA'AH+ @C3?>\"@W@JA&AJ+!@C&?>M@T@IA+AK+\"@EH?=-@T@IA-AE+#@C1?>N@S@IA&AI+$@DM?>J@T@IA*AE+%@B8?>6@R@HA'AG+&@AM??_@P@HA_AJ+'@B[?>/@U@IA AK+(@@=??_@R@HA A.+)@@5??<@P@HAWBD+*@@%@@M@O@GAWBI++@@S@@F@P@GA!BG+,??9@@N@Q@GA#A8+-@@G@@@@Q@HA)A@+.??-??2@Q@GA.@3+/@@S??-@P@GA*@.+0@@V??/@Q@HA#@*");
//	std::string data3("\"D1D~A8@NI@NH@NG@NF@NE@DGF");
//	std::string data3("\"D1A,TT@GK@GK@GK@GK@GK@GK@GK@GL@GL@GK@GK@GK@@@@@@@@@@@@M");
//	std::string data3("\"D1A,T>@A5@A5@A5@A5@A5@A5@A5@A5@A5@A5@A5@A5@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@A0@A0@A0@A0@A0@A0@A0@A0@C?@C>@C=@C<@C;@C:@C9@C7J");
//	std::string data3("\"D1A,UBAR.AT1AV3AX3AZ-A\"A^TA_?A!\A#C@AY@ID@@@@U\"");
//	std::string data3(" D7.E&\\@YRF@@B ZD65E'I@YHF@@B LD5");
	std::string data3("\"DOO@@@@$LK");
//	std::string data3(" D8IE,");
//	std::string data4("  :HG 1 #5 0.077 0.077 0.077 0.077 0.077 0.077 0.077 0.077 0.077 0.077 0.077 0.077 :VB 36 #60 13.6");
//	std::string data5("\"MJ0.41 12.48 442.60 13.13MJ0.41 12.52 442.60 13.21"); 
	std::string data5(" MJ100 MJ099 MJ100 MJ098 MJ100 MJ00.5 MJ00.5 MJ01.1 MJ00.5 MJ01.9 MJ028.5 MJ023 MJ017.5 MJ019 MJ001.0 MJ003.3 MJ05659.9 MJ14.2 MJ13.2 MJ14.3 MJ0.0 MJ20.3 MJ0.0 MJ");
	std::string data6("\":HG 7 #15 2.94 2.96 2.96 2.96 2.96 2.96 2.95 2.92 :BL 14.08");
	std::string data7("\"P85452401A1A>K@@^0T6V1AJ=@E@AFAH>AJZ4AM5BB6\"D<BL2@*.@E@\"@+G<BC -TV\"SAHAB@<@7@3@,");
	std::string data8("\"GG@KQ@:&@_^@EV]TL ");
	std::string data9("\"C1");
	std::string data10("");
	std::string data11("\"");
	//std::string data12("\"BT<@W\\DVGD$AES)DP*G:(NY$F6HB\\_F^HFSGFK!F]SF^\"FT)FM;F\\:F]9FTWFM;F\\C");
	std::string data12("\"BT<@U8DUCGU)D?*EJXF!]JO'JO'F@@JO'JO'JO'JO'F@@F@@F@@JO'JO'JO'F@@F@@F@@JO'JO'JO'");

	std::string buf;
	buf=sutron(data12);
	cout<<buf<<endl;

	return 0;
}
#include "sutron.cc"
