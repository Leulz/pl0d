
   /******************** codegen.h ********************/
#define MAXARRAY 11         /* The maximum length of arrays */

typedef enum codes{			/* Constants for operation codes (opecodes) */
	lit, opr, lod, lodpar, sto, cal, ret, ict, jmp, jpc, starr, lodar, stelar, var
}OpCode;

typedef enum ops{			/* Constants for operators */
	neg, add, sub, mul, div, odd, eq, ls, gr,
	neq, lseq, greq, wrt, wrl, wrtc
}Operator;

typedef struct var{
	union {
		int num, arr[MAXARRAY];
	}u;
}Variable;

int genCodeV(OpCode op, int v);		/* It generates a code with an address v. e.g. jmp, jpc and so on. */
int genCodeT(OpCode op, int ti);		/* It generates a code with an operand pointed by an index ti in the name table. e.g. lod, sto, and cal.*/
int genCodeO(Operator p);			/* It generates a code of an operator p*/
int genCodeR();					/*��It generates a return code */
int genCodeLarr(int ti, OpCode op);
void backPatch(int i);			/* It puts an address where a next code will be genereted into a code at an address i. */

int nextCode();			/* It returns an address where a next code will be generated. */
void listCode();			/* It lists generated codes. */
void execute();			/* It executes generated codes */

