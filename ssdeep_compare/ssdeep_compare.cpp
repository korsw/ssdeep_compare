#pragma warning(disable:4996)
#include <cstdio>
#include <limits>
#include <iostream>
#include "fileloading.h"

#define FFUZZYPP_DECLARATIONS

// the following line will activate assertions.
//#define FFUZZYPP_DEBUG

#include "ffuzzy.hpp"
using namespace ffuzzy;

// the following line will enable constant file size optimization.
#define OPTIMIZE_CONSTANT_FILE_SIZE

typedef union
{
	digest_unorm_t u;
	digest_t d;
} unified_digest_t;
typedef union
{
	digest_long_unorm_t u;
	digest_long_t d;
} unified_digest_long_t;

int comparehash(std::string hash1, std::string hash2);
int muticompare(std::string hash1, std::string* hash2, int numfile);


int main(int argc, char** argv)
{
	// digest_generator is reset by default.
	digest_generator gen;
	// Use "unnormalized" form not to remove
	// sequences with 4 or more identical characters.
	// You can use "digest_t" and you will get digests without such sequences.
	digest_unorm_t d;
	// Generated digest is always natural.
	// You don't have to give too large sizes.
	char digestbuf[digest_unorm_t::max_natural_chars];
	// File buffer	
	static const size_t BUFFER_SIZE = 4096;
	unsigned char filebuf[BUFFER_SIZE];

	int numfile = 0;
	std::string filenamelist[9999];								// 두번째 피연산자 파일 목록
	std::string operand_name = load_programname(argv[1]);		// 첫번째 피연산자
	load_programname(argv[0]);									// 실행파일 이름 초기화
	filelist(&numfile, &filenamelist[0]);						// 파일 리스트 생성
	std::string ssdeep[9999];									// 피연산자 ssdeep 목록
	std::string operand_ssdeep;									// 첫번째 피연산자 ssdeep


	// The buffer which contains the format string.
	// Converting to "unsigned" is completely safe for
	// ssdeep-compatible configuration
	// (I mean, unless you haven't modified many parameters).
	static_assert(digest_unorm_t::max_natural_width <= std::numeric_limits<unsigned>::max(),
		"digest_unorm_t::max_natural_width must be small enough.");
	char digestformatbuf[digest_unorm_t::max_natural_width_digits + 9];
	sprintf(digestformatbuf, "%%-%us  %%s\n",
		static_cast<unsigned>(digest_unorm_t::max_natural_width));
#if 0
	fprintf(stderr, "FORMAT STRING: %s\n", digestformatbuf);
#endif

	// iterate over all files given
	for (int i = 0; i < numfile; i++)
	{
		const char* filename = filenamelist[i].c_str();
		FILE* fp = fopen(filename, "rb");

		// error when failed to open file
		if (!fp)
		{
			perror(filename);
			return 1;
		}

#ifdef OPTIMIZE_CONSTANT_FILE_SIZE
		/*
			Retrieve file size (but the file is not seekable, do nothing).
			Note that using off_t is not safe for 32-bit platform.
			Use something much more robust to retrieve file sizes.
		*/
		bool seekable = false;
		off_t filesize;
		if (fseek(fp, 0, SEEK_END) == 0)
		{
			/*
				If seekable, set file size constant for
				digest generator optimization. The generator will work without it
				but using it makes the program significantly faster.
			*/
			seekable = true;
			filesize = ftell(fp);
			if (fseek(fp, 0, SEEK_SET) != 0)
			{
				fprintf(stderr, "%s: could not seek to the beginning.\n", filename);
				fclose(fp);
				return 1;
			}
			/*
				set_file_size_constant fails if:
				*	set_file_size_constant is called multiple times (not happens here)
				*	given file size is too large to optimize
			*/
			
		}
		else
		{
#if 0
			fprintf(stderr, "%s: seek operation is not available.\n", filename);
#endif
		}
#endif

		// update generator by given file stream and buffer
		if (!gen.update_by_stream<BUFFER_SIZE>(fp, filebuf))
		{
			fprintf(stderr, "%s: failed to update fuzzy hashes.\n", filename);
			fclose(fp);
			return 1;
		}
		fclose(fp);

		/*
			copy_digest will fail if:
			*	The file was too big to process
			*	The file size did not match with file size constant
				(set by set_file_size_constant)
		*/
		if (!gen.copy_digest(d))
		{
			if (gen.is_total_size_clamped())
				fprintf(stderr, "%s: too big to process.\n", filename);
#ifdef OPTIMIZE_CONSTANT_FILE_SIZE
			else if (seekable && (digest_filesize_t(filesize) != gen.total_size()))
				fprintf(stderr, "%s: file size changed while reading (or arithmetic overflow?)\n", filename);
#endif
			else
				fprintf(stderr, "%s: failed to copy digest with unknown error.\n", filename);
			return 1;
		}

		/*
			If size of the digest buffer is equal or greater than max_natural_chars,
			copying the string digest with pretty_unsafe function is completely safe.

			However, this function still may fail due to failure of sprintf.
		*/
		if (!d.pretty_unsafe(digestbuf))
		{
			fprintf(stderr, "%s: failed to stringize the digest.\n", filename);
			return 1;
		}
		printf(digestformatbuf, digestbuf, filename);
		//ssdeep[i] = digestformatbuf;
		ssdeep[i] = digestbuf;
		//std::cout << filename << std::endl;
		//std::cout << operand_name << std::endl;
		if (filename == operand_name)
		{
			operand_ssdeep = ssdeep[i];
		}
		// reset the generator if we haven't reached the end
		if (i + 1 != argc)
			gen.reset();
	}
	//std::cout << ssdeep[1] << std::endl;
	//std::cout << ssdeep[2] << std::endl;
	muticompare(operand_ssdeep, &ssdeep[0], numfile);
	return 0;
}

int muticompare(std::string operand_ssdeep, std::string* ssdeep, int numfile)
{
	//std::cout << operand_ssdeep << std::endl;

	for (int i = 0; i < numfile; i++)
	{
		std::string hash2 = *(ssdeep + i);
		comparehash(operand_ssdeep, hash2);
	}
	return 0;
}


int comparehash(std::string hash1, std::string hash2)
{
	const char* first_hash = hash1.c_str();
	const char* second_hash = hash2.c_str();

#if 1
	unified_digest_t h1, h2;
#else
	unified_digest_long_t h1, h2;
#endif
	char digestbuf[decltype(h1.u)::max_natural_chars];

	// Parse digests
	if (!decltype(h1.u)::parse(h1.u, first_hash))
	{
		fprintf(stderr, "error: failed to parse HASH1.\n");
		return 1;
	}
	if (!decltype(h2.u)::parse(h2.u, second_hash))
	{
		fprintf(stderr, "error: failed to parse HASH2.\n");
		return 1;
	}

	/*
		Restringize digests (just for demo)
		Notice that we're using h1.d instead of h1.u?
		This is not a good example but works perfectly.
	*/
	if (!h1.d.pretty_unsafe(digestbuf))
	{
		fprintf(stderr, "abort: failed to re-stringize HASH1.\n");
		return 1;
	}
	//printf("HASH1 : %s\n", digestbuf);
	if (!h2.d.pretty_unsafe(digestbuf))
	{
		fprintf(stderr, "abort: failed to re-stringize HASH2.\n");
		return 1;
	}
	//printf("HASH2 : %s\n", digestbuf);

	// Normalize digests and restringize them
	decltype(h1.d)::normalize(h1.d, h1.u);
	decltype(h2.d)::normalize(h2.d, h2.u);
	if (!h1.d.pretty_unsafe(digestbuf))
	{
		fprintf(stderr, "abort: failed to re-stringize HASH1.\n");
		return 1;
	}
	//printf("NORM1 : %s\n", digestbuf);
	if (!h2.d.pretty_unsafe(digestbuf))
	{
		fprintf(stderr, "abort: failed to re-stringize HASH2.\n");
		return 1;
	}
	//printf("NORM2 : %s\n", digestbuf);

	/*
		Compare them
		"Unnormalized" form has compare function but slow because of additional normalization)

		Note:
		Use `compare` or `compare<comparison_version::latest>` for latest
		version and `compare<comparison_version::v2_9>` for version 2.9 emulation.
	*/
	digest_comparison_score_t score =
		decltype(h1.d)::compare<comparison_version::v2_9>(h1.d, h2.d);
	printf("SCORE: %u\n", unsigned(score)); // safe to cast to unsigned (value is in [0,100])

	return 0;
}
