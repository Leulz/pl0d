
   /*********table.c**********/
#include <string.h>
#ifndef TBL
#define TBL
#include "table.h"
#endif
#include "getSource.h"

#define MAXTABLE 100		/* The maximum length of the name table */
#define MAXNAME  31		/* The maximum length of names */
#define MAXLEVEL 5		/* The maximum nesting block level */
#define MAXARRAY 10

typedef struct tableE {		/* The structure of elements in the name table */
	KindT kind;			/* Kinds of names */
	char name[MAXNAME];	/* The name */
	union {
		int value;			/* A value if it is an integer constant */
		struct {
			RelAddr raddr;	/*　The starting address if it is a function */
			int pars;		/*　The number of parameters if it is a function */
		}f;
		RelAddr raddr;		/*　The address if it is a variable or a parameter */
		int array[MAXARRAY + 1]; /* The array, if it is an array. The first value is the size of the array. */
	}u;
}TabelE;

static TabelE nameTable[MAXTABLE];		/* The name table */
static int tIndex = 0;			/*　An idex of the name table */
static int level = -1;			/*　The current block level */
static int lastIndex[MAXLEVEL];   	/* index[i] is the last index in the block level i */
static int addr[MAXLEVEL];    	/* addr[i] is the address of the last variable in the block level i */
static int localAddr;			/* The address of the last variable in the current block */
static int tfIndex;

static char* kindName(KindT k)		/* It returns the string of a kind of names */
{
	switch (k){
	case varId: return "var";
	case parId: return "par";
	case funcId: return "func";
	case constId: return "const";
	}
}

void blockBegin(int firstAddr)	/* It is called when a block starts. */
{
	if (level == -1){			/* It initializes variables if it is the main block. */
		localAddr = firstAddr;
		tIndex = 0;
		level++;
		return;
	}
	if (level == MAXLEVEL-1)
		errorF("too many nested blocks");
	lastIndex[level] = tIndex;		/* It preserves information of the previous block. */
	addr[level] = localAddr;
	localAddr = firstAddr;		/* The address of the first variable in the new block */
	level++;				/* The level of the new block */
	return;
}

void blockEnd()				/* It is called when a block ends. */
{
	level--;
	tIndex = lastIndex[level];		/* It restores information of one more outside block. */
	localAddr = addr[level];
}

int bLevel()				/* It returns the level of the current block. */
{
	return level;
}

int fPars()					/* It returns the number of parameters of the current block. */
{
	return nameTable[lastIndex[level-1]].u.f.pars;
}

void enterT(char *id)			/* It records a name id in the name table. */
{
	if (tIndex++ < MAXTABLE){
		strcpy(nameTable[tIndex].name, id);
	} else 
		errorF("too many names");
}
		
int enterTfunc(char *id, int v)		/* It records the name and the starting address of a function in the name table. */
{
	enterT(id);
	nameTable[tIndex].kind = funcId;
	nameTable[tIndex].u.f.raddr.level = level;
	nameTable[tIndex].u.f.raddr.addr = v;  		 /* The starting address of a function */
	nameTable[tIndex].u.f.pars = 0;  			 /* The initial number of parameters */
	tfIndex = tIndex;
	return tIndex;
}

int enterTpar(char *id)				/* It records parameters in the name table. */
{
	enterT(id);
	nameTable[tIndex].kind = parId;
	nameTable[tIndex].u.raddr.level = level;
	nameTable[tfIndex].u.f.pars++;  		 /* It updates the number of parameters. */
	return tIndex;
}

int enterTvar(char *id)			/* It records a variable name in the name table. */
{
	enterT(id);
	nameTable[tIndex].kind = varId;
	nameTable[tIndex].u.raddr.level = level;
	nameTable[tIndex].u.raddr.addr = localAddr++;
	nameTable[tIndex].u.raddr.offset = 0;
	return tIndex;
}

int enterTconst(char *id, int v)		/* It records an integer constant and its value in the name table. */
{
	enterT(id);
	nameTable[tIndex].kind = constId;
	nameTable[tIndex].u.value = v;
	return tIndex;
}

int enterTcharConst(char *id, int ch) /* It records a character constant and its character in the name table. */
{
	enterT(id);
	nameTable[tIndex].kind = constCharId;
	nameTable[tIndex].u.value = ch;
	return tIndex;
}

int enterTarrayConst(char *id, int arr[], int arrSize)
{
	int i;
	enterT(id);
	nameTable[tIndex].kind = constArrayId;
	nameTable[tIndex].u.array[0] = arrSize;
	for (i = 1; i < arrSize + 1; ++i)
	{
		nameTable[tIndex].u.array[i] = arr[i - 1];
	}
	return tIndex;
}

void endpar()					/* It is called when parameters end. */
{
	int i;
	int pars = nameTable[tfIndex].u.f.pars;
	if (pars == 0)  return;
	for (i=1; i<=pars; i++)			/* It calculates relative addresses of parameters. */
		 nameTable[tfIndex+i].u.raddr.addr = i-1-pars;
}

void changeV(int ti, int newVal)		/* It changes the value of the ti-th element in the name table (the starting address of a function) */
{
	nameTable[ti].u.f.raddr.addr = newVal;
}

int searchT(char *id, KindT k)		/* It returns the index of an element whose name is id. */
							/* It declares an error if it is undefined. */
{
	int i;
	i = tIndex;
	strcpy(nameTable[0].name, id);			/* It puts a sentinel at the beginning of the name table. */
	while( strcmp(id, nameTable[i].name) )
		i--;
	if ( i )							/* It finds the name. */
		return i;
	else {							/* It fails to find the name. */
		errorType("undef");
		if (k==varId) return enterTvar(id);	/* It records id in the name table if it is a variable. */
		return 0;
	}
}

int searchTbyId(char *id) /* It returns the index of an element whose name is id. Only used for searches, check 'searchT' */
{
	int i;
	i = tIndex;
	strcpy(nameTable[0].name, id);			/* It puts a sentinel at the beginning of the name table. */
	while( strcmp(id, nameTable[i].name) )
		i--;
	if ( i )							/* It finds the name. */
		return i;
	else {							/* It fails to find the name. */
		errorType("undef");
		return -1;
	}
}

KindT kindT(int i)				/* It returns the kind of the i-th element in the name table. */
{
	return nameTable[i].kind;
}

RelAddr relAddr(int ti)				/* It returns the address of the i-th element in the name table. */
{
	return nameTable[ti].u.raddr;
}

int val(int ti)					/* It returns the value of the i-th element in the name table. */
{
	return nameTable[ti].u.value;
}

int pars(int ti)				/* It returns the number of parameters of a function (the i-th element in the name table). */
{
	return nameTable[ti].u.f.pars;
}

int frameL()				/* The maximum relative address of variables in a block */
{
	return localAddr;
}

int getArrayElement(int ti, int arrayIndex)
{
	return nameTable[ti].u.array[arrayIndex + 1];
}

/*int getVarArrayElement(int ti, int arrayIndex)
{
	return nameTable[ti].u.v.array[arrayIndex + 1];
}

void setVarArrayElement(int ti, int arrayIndex, int value)
{
	nameTable[ti].u.v.array[arrayIndex + 1] = value;
}

void setKindT(int ti, KindT kind)
{
	nameTable[ti].kind = kind;
}

void setVarArray(int ti, int arraySize, int array[])
{
	int i;
	nameTable[ti].u.v.array[0] = arraySize;
	for (i = 1; i < arraySize + 1; ++i)
	{
		nameTable[ti].u.v.array[i] = array[i-1];
	}
}*/