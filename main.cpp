#include <immintrin.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <functional>
#include <chrono>
#include <cassert>
#include <random>
#include <omp.h>

//https://arxiv.org/ftp/arxiv/papers/1212/1212.2207.pdf

const std::string pendl = ".\n";

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

double time(const std::function<void()> &f) {
	f(); //warmup call
	auto start = std::chrono::system_clock::now();
	f();
	auto stop = std::chrono::system_clock::now();
	//return the runtime of the algorithm in seconds
	return std::chrono::duration<double>(stop - start).count();
}

unsigned char reverse_bits(unsigned char n) {
	unsigned char ret = n;
	int i = 0;
	for(;;) {
		if(i == 8) break;
		ret <<= 1; //shift ret left by 1 bit

		if((n & 1) == 1) {
			ret ^= 1; //if current bit is set, toggle it
		}
		n >>= 1; //shift n to the right by 1 bit

		i++;
	}

	return ret;
}

std::string bytes_to_message(unsigned char* data) {
	//char char_rep[8];
	std::string ret;
	size_t i = 0;
	//unsigned int decoded_char = 0;
	for(;;) {
		//std::cout << "Pass #" << i << std::endl;
		unsigned char decoded_char = 0;
		size_t index = 8 * i;
		std::vector<char> rep;
		//std::cout << "Byte rep: ";
		for(size_t j = index; j < index + 8; j++) {
			rep.push_back(data[j] & 1);
			//std::cout << (data[j] & 1) << " ";
		}
		//std::cout << std::endl;
		for(size_t k = 0; k < 8; k++) {
			decoded_char |= (rep[k] << k);
		}
		//std::cout << "Decoded char: " << (char)reverse_bits(decoded_char) << std::endl;
		ret.push_back((char)reverse_bits(decoded_char));
		if((unsigned char)reverse_bits(decoded_char) == '\0') break;
		i++;
	}
	return ret;
}

const unsigned char PNG_HEADER[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

int main(int argc, char** argv) {
    if(argc > 4 || argc < 2) {
        std::cout << RED << "Usage: wav-steg <audio.wav> <message.txt> <output.wav>" << RESET << pendl;
		//std::cout << "64 reversed is " << reverse_bits(64) << std::endl;
        return 0;
    }
	else if(argc == 2) {
		auto input_name = (const char *)argv[1];
        std::ifstream input_file(input_name, std::ios::in | std::ios::binary);
        input_file.seekg(0, std::ios::end);
        size_t size = input_file.tellg();
        std::cout << "Size of " << input_name << " (with header): " << size << pendl;
        input_file.seekg(0, std::ios::beg);

        char header[44]; //wav file header is 44 bytes in length, we will just skip past it and write it to file later
        input_file.read(header, 44);

		unsigned char* data = new unsigned char[size - 44 + 1];
        input_file.read((char*)data, size - 44);
        data[size] = '\0';
        input_file.close();

		std::cout << "=== Beginning decoding ===" << std::endl;
		std::string output = bytes_to_message(data);
		std::cout << "Decoded: " << output << std::endl;

		delete data;
	}
    else if (argc == 4) {

        /*
        *
        * READING BYTES OF INPUT PNG AND STORING BYTES ON THE HEAP TO BE MODIFIED LATER
        *
        */


        auto input_name = (const char *)argv[1];
        std::ifstream input_file(input_name, std::ios::in | std::ios::binary);
        input_file.seekg(0, std::ios::end);
        size_t size = input_file.tellg();
        std::cout << "Size of " << input_name << " (with header): " << size << pendl;
        input_file.seekg(0, std::ios::beg);

        char header[44]; //wav file header is 44 bytes in length, we will just skip past it and write it to file later
        input_file.read(header, 44);
        /*for(int i = 0; i < 8; ++i) {
            std::cout << "Byte " << i << ": " << header[i] << std::endl;
            if(header[i] != PNG_HEADER[i]) {
                std::cout << "ERROR: " << input_name << " is not a valid .png file" << std::endl;
                input_file.close();
                exit(1);
            }
        }*/
        //char* data = new char[size - 8 + 1];
		char* data = new char[size - 44 + 1];
        input_file.read(data, size - 44);
        data[size] = '\0';
        input_file.close();

        /*
        *
        * READING IN MESSAGE FILE FROM COMMAND LINE AND STORING INTO AN STD::STRING CALLED 'message'
        *
        */

        std::ifstream msg_file(argv[2]);
        //std::string message;
        msg_file.seekg(0, std::ios::end);
        size_t message_size = msg_file.tellg();
        std::cout << "Size of " << argv[2] << ": " << message_size << pendl;
        std::string message(message_size + 1, ' ');
        msg_file.seekg(0, std::ios::beg);
        msg_file.read(&message[0], message_size);
		message[message_size] = '\0';
        msg_file.close();
        std::cout << "Message: \n" << message << std::endl;
		if(message.size() * 8 > size - 44) {
			std::cout << "ERROR: message too large to encode in input image." << pendl;
			exit(1);
		}
		std::cout << "=== Beginning encoding ===" << std::endl;

        /*
        *
        * FLIPPING LEAST SIGNIFICANT BIT IN EVERY 8 BYTES TO ACCOUNT FOR EACH CHAR IN THE MESSAGE
        *
        */
		#pragma omp parallel for num_threads(omp_get_max_threads()) shared(data)
		for(size_t i = 0; i < message.size(); ++i) {
			//loop through each character in the message string
			for(size_t j = 0; j < 8; j++) {
				unsigned int val = message[i] >> (7 - j);
				val &= 1;
				size_t index = (8 * i) + j;
				unsigned int _data = data[index] & !(1);
				data[index] = (_data | val);
			}
		}
		std::cout << "Message successfully encoded, writing to " << argv[3] << pendl;

		std::ofstream output_file(argv[3], std::ios::binary);
		output_file.write(header, 44);
		output_file.write(data, size - 44);
		output_file.close();

		delete data;
    }
    return 0;
}
