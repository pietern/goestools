/*HART makes limited use of data compression in the form of Packed ASCII. Normally, there are 256 possible ASCII characters, so that a full byte is needed to represent a character. Packed ASCII is a subset of full ASCII and uses only 64 of the 256 possible characters. These 64 characters are the capitalized alphabet, numbers 0 through 9, and a few punctuation marks. Many HART parameters need only this limited ASCII set, which means that data can be compressed to 3/4 of normal. This improves transmission speed, especially if the textual parameter being communicated is a large one.

Since only full bytes can be transmitted, the 3/4 compression is fully realized only when the number of uncompressed bytes is a multiple of 4. Any fractional part requires a whole byte. Thus, if U is the number of uncompressed bytes, and T the number of transmitted bytes; find T = (3*U)/4 and increase any fractional part to 1. As examples, U = 3, 7, 8, and 9 result in T = 3, 6, 6, and 7.

The rule for converting from ASCII to Packed ASCII is just to remove bits 6 and 7 (two most significant). An example is the character "M". The full binary code to represent this is 0100,1101. The packed binary code is 00,1101. The rules for conversion from packed ASCII back to ASCII are (1) set bit 7 = 0 and (2) set bit 6 = complement of packed ASCII bit 5.
*/
unsigned long packed;
read(device,&packed,3);
char unpacked[5]={0};

for(int i=0;i<4;++i)
{
  unpacked[i]=packed>>(6*(3-i)) & 0x3F;   // Divide into blocks of 6
  char bit6 = (~unpacked[i] & 0x10) << 1; // set bit 6 = complement of packed ASCII bit 5.
  unpacked[i]|=bit6;                      // bit 6 is set, bit 7 is set to 0 from the start.
}

