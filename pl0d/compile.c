
   /*************** compile.c *************/

#include "getSource.h"
#include <string.h>
#ifndef TBL
#define TBL
#include "table.h"
#endif
#include "codegen.h"

#define MINERROR 3			/* It will execute object codes if the number of errors in the compilation is less than MINERROR. */
#define FIRSTADDR 2			/* The address of the first variable of each block. */

static Token token;				/* The next token */

static void block(int pIndex);	/* It compiles a block */
						/* pIndex is the index of the function name of this block */
static void constDecl();			/* It compiles a constant declaration. */
static void varDecl();				/* It compiles a variable declaration. */
static void funcDecl();			/* It compiles a function declaration. */
static void statement();			/* It compiles a statement. */
static Type expression();			/* It compiles an expression. */
static Type term();				/* It compiles a term of an expression. */
static Type factor();				/* It compiles a fcator of an expression. */
static void condition(int invert);			/* It compiles a conditional expression. */
static int isStBeginKey(Token t);		/* Is a token t one of starting tokens of statements? */

//Put this in a helper.c/util.c ?
/* Used for getting the string representation of a KeyId value. Useful for debugging. */
char* getKeyIdName(KeyId k) 
{
	switch(k) {
		case Call:
			return "Call";
		case Begin:
			return "Begin";
		case End:
			return "End";
		case If:
			return "If";
		case Then:
			return "Then";
		case While:
			return "While";
		case Do:
			return "Do";
		case Ret:
			return "Ret";
		case Func:
			return "Func";
		case Var:
			return "Var";
		case Const:
			return "Const";
		case Write:
			return "Write";
		case WriteLn:
			return "WriteLn";
		case end_of_KeyWd:
			return "end_of_KeyWd";
		case Plus:
			return "Plus";
		case Minus:
			return "Minus";
		case Mult:
			return "Mult";
		case Div:
			return "Div";
		case Apostrophe:
			return "Apostrophe";
		case Lparen:
			return "Lparen";
		case Rparen:
			return "Rparen";
		case Lbracket:
			return "Lbracket";
		case Rbracket:
			return "Rbracket";
		case Equal:
			return "Equal";
		case Lss:
			return "Lss";
		case Gtr:
			return "Gtr";
		case NotEq:
			return "NotEq";
		case LssEq:
			return "LssEq";
		case GtrEq:
			return "GtrEq";
		case Comma:
			return "Comma";
		case Period:
			return "Period";
		case Semicolon:
			return "Semicolon";
		case Assign:
			return "Assign";
		case end_of_KeySym:
			return "end_of_KeySym";
		case Id:
			return "Id";
		case Num:
			return "Num";
		case nul:
			return "nul";
		case end_of_Token:
			return "end_of_Token";
		case letter:
			return "letter";
		case digit:
			return "digit";
		case colon:
			return "colon";
		case character:
			return "character";
		case others:
			return "others";
		default:
			return "Not identified.";
	}
}

int compile()
{
	int i;
	printf("; start compilation\n");
	initSource();				/* Initialization for getSource */
	token = nextToken();			/* The first token */
	blockBegin(FIRSTADDR);		/* A new block starts. */
	block(0);					/* The constatnt "0" is a dummy. (The main block does not have any function name.) */
	finalSource();
	i = errorN();				/* The number of error messages */
	if (i!=0)
	  printf("; %d errors\n", i);
	//listCode();			/* It lists object codes if needed. */
	return i<MINERROR;		/* Is the number of error messages acceptable so as to execute the object code? */
}

//Makes non-function declarations separately, since function declarations should be skipped when the code is being executed.
void declarations()
{
	while (1) {
		switch (token.kind) {
			case Const:			/* A constant declaration */
				token = nextToken();
				constDecl(); continue;
			case Var:				/* A variable declaration */
				token = nextToken();
				varDecl(); continue;
			default:
				break;
		}
		break;
	}
}

void block(int pIndex)		/* pIndex is the index of the function name of this block */
{
	int backP;
	declarations();
	backP = genCodeV(jmp, 0);		/* It generates a jmp to skip internal functions. The address will be adjusted by backpatch. */
	//note: functions must be declared sequentially
	while (1) {				/* It repeatedly compiles declarations. */
		switch (token.kind){
		case Func:				/* A function declaration */
			token = nextToken();
			funcDecl(); continue;
		default:				/* Otherwise, it is the end of declarations. */
			break;
		}
		break;
	}			
	backPatch(backP);			/* It adjusts the target address of the jmp to skip internal functions. */
	declarations();
	changeV(pIndex, nextCode());	/* It adjusts the starting address of this function. */
	genCodeV(ict, frameL());		/* A code to occupy the frame of this block on the stack. */
	statement();				/* The main statement of this block */		
	genCodeR();				/* The return code */
	blockEnd();				/* It declares the end of a block to the name table. */
}	

void constDecl()			/* It compiles a constant declaration. */
{
	Token temp;
	int flag = 0, arrayFlag = 0;
	while(1){
		if (token.kind==Id){
			setIdKind(constId);				/* It sets the kind of the token for printing. */
			temp = token; 					/* It records the name of the token. */
			token = checkGet(nextToken(), Equal);		/* The next token should be "=". */
			switch(token.kind) {
				case Num:
					enterTconst(temp.u.id, token.u.value);	/* It records both the constant name and its value in the name table. */
					break;
				case character:
					enterTcharConst(temp.u.id, token.u.value);
					break;
				case Lbracket:
					arrayFlag = 1;
					token = nextToken();
					int arraySize = 0, array[10], switchFlag = 0;
					while(1 && !switchFlag) {
						switch(token.kind) {
							case Num:
								if(arraySize < 10) {
									array[arraySize++] = token.u.value;
								} else {
									arraySize++;
								}
							case Comma:
								token = nextToken();
								continue;
							default:
								switchFlag = 1;
								token = checkGet(token, Rbracket);
								break;
						}
					}
					enterTarrayConst(temp.u.id, array, arraySize);
					if (arraySize > 10){
						errorMessage("array is too big. only first 10 elements considered");
					}
					break;
				default:
					errorType("number nor char");
					break;
			}
			if (flag) {
				setcTokenKind(character);
				setcTokenValue(token.u.id[0] - '0');
				flag = 0;
			}
			if (arrayFlag) {
				arrayFlag = 0;
			} else {
				token = nextToken();				
			}
		}else
			errorMissingId();
		if (token.kind!=Comma){		/* If the next token is a comma, it will be followed by a constant declaration. */
			if (token.kind==Id){	/* If the next token is a name, it assumes there must be a comma. */
				errorInsert(Comma);
				continue;
			}else
				break;
		}
		token = nextToken();
	}
	token = checkGet(token, Semicolon);		/* It must end with ";". */
}

void varDecl()				/* It compiles a variable declaration. */
{
	while(1){
		if (token.kind==Id){
			setIdKind(varId);		/* It sets the kind of the token for printing. */
			int ti = enterTvar(token.u.id);		/* It records the name of a variable in the name table whose address will be determined by the name table. */
			genCodeT(var, ti);
			token = nextToken();
		}else
			errorMissingId();
		if (token.kind!=Comma){		/* If the next token is a comma, it will be followed by a variable declaration. */
			if (token.kind==Id){	/* If the next token is a name, it assumes there must be a comma. */
				errorInsert(Comma);
				continue;
			}else
				break;
		}
		token = nextToken();
	}
	token = checkGet(token, Semicolon);		/* It must end with ";". */
}

void funcDecl()			/* It compiles a function declaration. */
{
	int fIndex;
	if (token.kind==Id){
		setIdKind(funcId);				/* It sets the kind of the token for printing. */
		fIndex = enterTfunc(token.u.id, nextCode());		/* It records the function name in the name table. */
				/* It records the address of the next code as the starting address of this function at first. */
		token = checkGet(nextToken(), Lparen);
		blockBegin(FIRSTADDR);	/* The level of parameters is the same as the level of the block of the function. */
		while(1){
			if (token.kind==Id){			/* The function has parameters. */
				setIdKind(parId);		/* It sets the kind of the token for printing. */
				enterTpar(token.u.id);		/* It records the parameters in the name table. */
				token = nextToken();
			}else
				break;
			if (token.kind!=Comma){		/* If the next token is a comma, it will be followed by a parameter. */
				if (token.kind==Id){		/* If the next token is a name, it assumes there must be a comma. */
					errorInsert(Comma);
					continue;
				}else
					break;
			}
			token = nextToken();
		}
		token = checkGet(token, Rparen);		/* It must ed with ")". */
		endpar();				/* It declares the end of parameters to the name table. */
		if (token.kind==Semicolon){
			errorDelete();
			token = nextToken();
		}
		block(fIndex);	/* It compiles a block. The parameter is the index of the function. */
		token = checkGet(token, Semicolon);		/* It must end with ";". */
	} else 
		errorMissingId();			/* No function name. */
}

void statement()			/* It compiles a statement. */
{
	Token temp;
	int tIndex;
	KindT k;
	int backP, backP2;	/* Variables to record addresses of codes whose address parts must be adjusted later */

	while(1) {
		switch (token.kind) {
			//FIXME This tIndex is only correct the first time. If you keep changing the type of the variable, the search won't work.
		case Id:					/* An assignment statement := */
			tIndex = searchT(token.u.id, token.kind);	/* The index of a left-hand variable */
			setIdKind(k=kindT(tIndex));		/* It sets the kind of the left-hand variable for printing. */
			if (k != varId && k != parId && k != varArrayId) 		/* The left-hand variable must be a variable or a parameter. */
				errorType("var/par/varArray");
			temp = token;
			token = nextToken();
			if (token.kind == Lbracket) {
				token = nextToken();
				expression();
				token = checkGet(token, Rbracket);
				token = checkGet(token, Assign);
				expression();
				genCodeLarr(tIndex, stelar);
			} else {
				token = checkGet(token, Assign);
				if (token.kind == Lbracket) {
					setKindT(tIndex, varArrayId);
					token = nextToken();
					int arraySize = 0, switchFlag = 0;
					while(1 && !switchFlag) {
						switch(token.kind) {
							case Num: case Id:
								arraySize++;
								if (arraySize <= 10) {
									expression();
								} else {
									while(1) {
										token = nextToken();
										if (token.kind == Rbracket || token.kind == Semicolon) {
											break;
										}
									}
								}
								continue;
							case Comma:
								token = nextToken();
								continue;
							default:
								switchFlag = 1;
								token = checkGet(token, Rbracket);
								break;
						}
					}
					genCodeV(lit, arraySize);
					genCodeT(starr, tIndex);
					if (arraySize > 10) {
						errorMessage("array is too big, only first 10 elements considered");
					}
				} else {
					Type type = expression();					/* It compiles an expression. */
					if (type.keyId == character) {
						setKindT(tIndex, varCharId);
					}
					genCodeT(sto, tIndex);	  /* A code to store the right-hand value in the left-hand variable */
				}
			}
			return;
		case If:					/* An if-statement */
			token = nextToken();
			condition(0);					/* A conditional expression */
			token = checkGet(token, Then);		/* It must be "then". */
			backP = genCodeV(jpc, 0);			/* A conditional jump */
			statement();					/* A statement just after "then" */
			backPatch(backP);				/* It adjusts the target address of the conditional jump. */
			if (token.kind == Else) {
				token = nextToken();
				backP2 = genCodeV(jmp, 0);
				backPatch(backP);
				statement();
				backPatch(backP2);
			}
			return;
		case Ret:					/* A return statement */
			token = nextToken();
			expression();					/* An expression */
			genCodeR();					/* A return code */
			return;
		case Begin:				/* A begin-end statement */
			token = nextToken();
			while(1){
				statement();				/* A statement */
				while(1){
					if (token.kind==Semicolon){		/* If the next token is ";", it will be followed by a statement. */
						token = nextToken();
						break;
					}
					if (token.kind==End){			/* If the next token is "end", it is the end of the begin-end statement. */
						token = nextToken();
						return;
					}
					if (isStBeginKey(token)){		/* If the next token is one of the starting symbols of statements, */
						errorInsert(Semicolon);	 /* It assumes there must be ";". */
						break;
					}
					errorDelete();	/* Otherwise, it declares an error and ignores the token.��*/
					token = nextToken();
				}
			}
		case While:				/*��A while-statement */
			token = nextToken();
			backP2 = nextCode();  /* The target address of the jump at the end of the while-statment. */
			condition(0);				/* A condiional expression */
			token = checkGet(token, Do);	/* It must be "do". */
			backP = genCodeV(jpc, 0);		/* A conditonal jump which jumps when the condition is false */
			statement();				/* A statement (the body of the while-statement) */
			genCodeV(jmp, backP2);		/* A jump to the beginning of the while-statement */
			backPatch(backP);	/* It adjusts the target address of the conditional jump */
			return;
		case Do:
			token = nextToken();
			backP = nextCode();
			statement();
			token = checkGet(token, While);
			condition(1);
			genCodeV(jpc, backP);
			return;
		case Repeat:
			token = nextToken();
			backP = nextCode();
			statement();
			token = checkGet(token, Until);
			condition(0);
			genCodeV(jpc, backP);
			return;
		case Write:			/* A write-statement */
			token = nextToken();
			Type end = expression();				/* An expression */
			switch(end.keyId) {
				case Id:
					switch(end.kindT){
						case constCharId: case varCharId:
							genCodeO(wrtc);
							break;
						default:
							genCodeO(wrt);				/* A code to write the value of the expression */
							break;
					}
					break;
				case character:
					genCodeO(wrtc);
					break;
				default:
					genCodeO(wrt);				/* A code to write the value of the expression */
					break;
			}
			return;
		case WriteLn:			/* A code to write a new line */
			token = nextToken();
			genCodeO(wrl);				/* A code to write a new line��*/
			return;
		case End: case Semicolon:			/* An empty statement */
			return;
		case Call:
			token = nextToken();
			tIndex = searchT(token.u.id, funcId);
			token = nextToken();
			if (token.kind==Lparen){
				int i=0; 					/* The number of arguments */
				token = nextToken();
				if (token.kind != Rparen) {
					for (; ; ) {
						expression(); i++;	/* It compiles an argument. */
						if (token.kind==Comma){	/* If the next token is a comma, it will be followed by an argument. */
							token = nextToken();
							continue;
						}
						token = checkGet(token, Rparen);
						break;
					}
				} else
					token = nextToken();
				if (pars(tIndex) != i) 
					errorMessage("\\#par");	/* pars(tIndex) is the number of parameters. */
			}else{
				errorInsert(Lparen);
				errorInsert(Rparen);
			}
			genCodeT(cal, tIndex);				/* A code to call a function */
			return;
		default:			      /* It ignores tokens preceeding a starting token of statements */
			errorDelete();				/* It ignores tokens. */
			token = nextToken();
			continue;
		}		
	}
}

int isStBeginKey(Token t)			/* Is a token t one of starting tokens of statements? */
{
	switch (t.kind){
	case Id:
	case If: case Begin: case Ret: case Do: case Repeat:
	case While: case Write: case WriteLn: case Call:
		return 1;
	default:
		return 0;
	}
}

Type expression()				/* It compiles an expression. */
{
	KeyId k;
	Type type;
	k = token.kind;
	if (k==Plus || k==Minus){
		token = nextToken();
		type = term();
		if (k==Minus)
			genCodeO(neg);
	}else{
		type = term();
	}
	k = token.kind;
	while (k==Plus || k==Minus){
		token = nextToken();
		term();
		if (k==Minus)
			genCodeO(sub);
		else
			genCodeO(add);
		k = token.kind;
	}
	return type;
}

Type term()					/* It compiles a term of an expression. */
{
	KeyId k;
	Type facType = factor();
	k = token.kind;
	while (k==Mult || k==Div){
		token = nextToken();
		factor();
		if (k==Mult)
			genCodeO(mul);
		else
			genCodeO(div);
		k = token.kind;
	}
	return facType;
}

Type factor()					/* It compiles a factor of an expression. */
{
	int tIndex, i, arrayElement;
	KindT kind;
	KeyId k = token.kind;
	Type type;
	Token temp;
	if (token.kind==Id){
		tIndex = searchT(token.u.id, varId);
		setIdKind(kind=kindT(tIndex));		/* It sets the kind of the identifier for printing. */
		switch (kind) {
		case varId:	case varCharId:		/* The name of a variable or the name of a parameter */
			genCodeT(lod, tIndex);
			token = nextToken(); break;
		case parId:
			genCodeT(lodpar, tIndex);
			token = nextToken(); break;
		case varArrayId:
			;
			temp = token;
			token = checkGet(nextToken(), Lbracket);
			tIndex = searchT(temp.u.id, varArrayId);
			expression();
			genCodeLarr(tIndex, lodar);
			token = checkGet(token, Rbracket);
			break;
		case constId:					/* The name of a constant */
			genCodeV(lit, val(tIndex));
			token = nextToken(); break;
		case constCharId:
			genCodeV(lit, (val(tIndex) + '0') - '0');
			token = nextToken(); break;
		case constArrayId:
			;
			temp = token;
			token = checkGet(nextToken(), Lbracket);
			if (token.u.value > 9) {
				errorF("index bigger than array size, core dumped");
			}
			tIndex = searchT(temp.u.id, constArrayId);
			arrayElement = getArrayElement(tIndex, token.u.value);
			genCodeV(lit, arrayElement);
			token = checkGet(nextToken(), Rbracket);
			break;
		case funcId:					/* A function call */
			token = nextToken();
			if (token.kind==Lparen){
				i=0; 					/* The number of arguments */
				token = nextToken();
				if (token.kind != Rparen) {
					for (; ; ) {
						expression(); i++;	/* It compiles an argument. */
						//genCodeT(sto, parTIndex);
						if (token.kind==Comma){	/* If the next token is a comma, it will be followed by an argument. */
							token = nextToken();
							continue;
						}
						token = checkGet(token, Rparen);
						break;
					}
				} else
					token = nextToken();
				if (pars(tIndex) != i) 
					errorMessage("\\#par");	/* pars(tIndex) is the number of parameters. */
			}else{
				errorInsert(Lparen);
				errorInsert(Rparen);
			}
			genCodeT(cal, tIndex);				/* A code to call a function */
			break;
		}
	}else if (token.kind==Num){			/* a constant */
		genCodeV(lit, token.u.value);
		token = nextToken();
	}else if (token.kind==Lparen){			/* '(' an expression ')' */
		token = nextToken();
		expression();
		token = checkGet(token, Rparen);
	} else if (token.kind==character){
		genCodeV(lit, token.u.value);
		token = nextToken();
		k = character;
	}

	switch (token.kind){					/* It declares an error if this factor is followed by a factor. */
	case Id: case Num: case Lparen:
		errorMissingOp();
		factor();
	default:
		type.keyId = k;
		type.kindT = kind;
		return type;
	}
}
	
void condition(int invert)					/* It compiles a conditional expression. */
{
	KeyId k;
	if (token.kind==Odd){
		token = nextToken();
		expression();
		genCodeO(odd);
	}else{
		expression();
		k = token.kind;
		switch(k){
		case Equal: case Lss: case Gtr:
		case NotEq: case LssEq: case GtrEq:
			break;
		default:
			errorType("rel-op");
			break;
		}
		token = nextToken();
		expression();
		if (!invert)
			switch(k){
			case Equal: genCodeO(eq); break;
			case Lss: genCodeO(ls); break;
			case Gtr: genCodeO(gr); break;
			case NotEq:	genCodeO(neq); break;
			case LssEq:	genCodeO(lseq); break;
			case GtrEq:	genCodeO(greq); break;
			}
		else
			switch(k){
			case Equal: genCodeO(neq); break;
			case Lss: genCodeO(greq); break;
			case Gtr: genCodeO(lseq); break;
			case NotEq:	genCodeO(eq); break;
			case LssEq:	genCodeO(gr); break;
			case GtrEq:	genCodeO(ls); break;
			}
	}
}