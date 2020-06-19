# Python 2.7
# Usage: python hextobin.py inputFile outputFile dontFlipEndianness
# Endianness: See https://en.wikipedia.org/wiki/Endianness
# dontFlipEndianness: If the input is unknown, 0 will flip the ordering, 1 will keep it consistent

import sys

if __name__ == "__main__":
    # STRICTLY do NOT modify the code in the main function here
    if len(sys.argv) != 4:
        print ("\nUsage: python hextobin.py inputFile outputFile dontFlipEndianness")
        print ("If the input is unknown, 0 will flip the ordering, 1 will keep it consistent")
        raise ValueError("Wrong number of arguments!")
    try:
        input_f = open(sys.argv[1], 'r')
    except IOError:
        print ("\nUsage: python hextobin.py inputFile outputFile dontFlipEndianness")
        print ("If the input is unknown, 0 will flip the ordering, 1 will keep it consistent")
        raise IOError("Input file not found!")

    # Inefficient but sufficient
    # Init lookup table for hex to bin
    map_byte_hextobin = {}

    # Build charset: http://www.asciitable.com/
    charset = []
    for i in range(48,58):
        charset.append(chr(i))
    for i in range(65,71):
        charset.append(chr(i))
    for i in range(97,103):
        charset.append(chr(i))
        
    # Build said lookup table
    for char1 in charset:
        for char2 in charset:
            map_byte_hextobin[char1+char2] = int(char1+char2, 16)
    
    # Note that the code expects it to be in little endian (this could depend on the machine!)
    # 
    with open(sys.argv[2], 'w') as output_f:
        buffer = bytearray([0,0,0,0, 0,0,0,0])
        # Big Endian
        if sys.argv[3] == "1":
            for line in input_f:
                # populate buffer
                for i in range(len(buffer)):
                    l_index = i*2
                    #MSB first for both, buffer[0] is MSByte etc
                    buffer[i] = map_byte_hextobin[line[l_index]+line[l_index+1]]
                output_f.write(buffer)
        elif sys.argv[3] == "0":
            # Small Endian
            for line in input_f:
                # populate buffer
                # words are in order, contents of bytes in order
                # ordering of bytes in word is reversed for each 32 bit int
                # for the 2 words
                for i in [0,4]:
                    line_i = i*2
                    buffer[i+0] = map_byte_hextobin[line[line_i+6]+line[line_i+7]]
                    buffer[i+1] = map_byte_hextobin[line[line_i+4]+line[line_i+5]]
                    buffer[i+2] = map_byte_hextobin[line[line_i+2]+line[line_i+3]]
                    buffer[i+3] = map_byte_hextobin[line[line_i+0]+line[line_i+1]]

                output_f.write(buffer)
        else:
            print ("\nUsage: python hextobin.py inputFile outputFile Endianness\nEndianness is 0 for Little & 1 for Big")
            print ("If the input is unknown, 0 will flip the ordering, 1 will keep it consistent")
            raise ValueError("Endianness must be 0 or 1!")