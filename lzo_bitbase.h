#include "lzo/minilzo.h"
#include "lzo/minilzo.c"


#define BDB_2_SIZE (2*65*64*64)
#define BDB_3_SIZE (2*65*64*64*64)
#define BDB_SIZE (BDB_2_SIZE + BDB_2_SIZE + BDB_2_SIZE + BDB_3_SIZE + BDB_3_SIZE + BDB_3_SIZE + BDB_3_SIZE + BDB_3_SIZE + BDB_3_SIZE)

signed char bitbase_dtm[BDB_SIZE]; // e.g.: bitbase_dtm[piece_config + position_index]
char bitbase_dtm_file[] = "bitbases.dtm";
// note - this large static array confuses valgrind :(


//
//
//

#define OUT_LEN     (BDB_SIZE + BDB_SIZE / 16 + 64 + 3)

// static unsigned char __LZO_MMODEL in  [ IN_LEN ];
static unsigned char __LZO_MMODEL bitbase_dtm_lzo [ OUT_LEN ];


/* Work-memory needed for compression. Allocate memory in units
 * of 'lzo_align_t' (instead of 'char') to make sure it is properly aligned.
 */

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);


void compress_bitbase()
	{
	int r;
	lzo_uint out_len = BDB_SIZE;

	r = lzo1x_1_compress(bitbase_dtm,BDB_SIZE,bitbase_dtm_lzo,&out_len,wrkmem);
	if (r != LZO_E_OK)
		printf("Compression failed, no bitbase support\n");

	if (out_len >= BDB_SIZE)
		printf("Incompressible data detected\n");

	printf("Compressed %d bytes to %d bytes\n", BDB_SIZE, out_len);

	FILE * save = fopen(bitbase_dtm_file, "w");
	fwrite(bitbase_dtm_lzo, sizeof(char), out_len, save);
	fclose(save);
	}

int decompress_bitbase()
	{
	int r;
	lzo_uint out_len;
	lzo_uint new_len;

	FILE * save = fopen(bitbase_dtm_file, "r");
	if (save)
		{
		fseek(save, 0, SEEK_END);
		out_len = ftell(save);
		fseek(save, 0, SEEK_SET);
		fread(bitbase_dtm_lzo, sizeof(char), out_len, save);
		r = lzo1x_decompress(bitbase_dtm_lzo,out_len,bitbase_dtm,&new_len,NULL);
		fclose(save);
		if (r == LZO_E_OK && new_len == BDB_SIZE)
			{
		        printf("Decompressed %lu bytes to %lu bytes\n",
	        	    (unsigned long) out_len, (unsigned long) BDB_SIZE);
			return 1;
			}
		else
			printf("Decompression failed, no bitbase support\n");
		}
	return 0;
	}
