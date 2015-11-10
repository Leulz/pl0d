char peekNextChar()				/* It returns the next character. */
{
	printf("Pelo menos entrou aqui? Line index = %d\n", lineIndex);
	char ch;
	int tempLineIndex = lineIndex, i;
	char tempLine[MAXLINE];	/* An input buffer for one line */
	//FILE *tempfpi = fdopen (dup (fileno (fpi)), "r");
	
	for (i = 0; i < MAXLINE; ++i)
	{
		tempLine[i] = line[i];
	}

	if (tempLineIndex == -1){
		printf("TEMP LINE INDEX == -1\n");
		if (fgets(tempLine, MAXLINE, fpi) != NULL){
			printf("DEU FGETS!!!!!\n");
/*			puts(line); */	/* FYI, ordinary error messages */
			tempLineIndex = 0;
		}
	}
	if ((ch = tempLine[tempLineIndex++]) == '\n'){	 /*¡¡ch gets the next character. */
		printf("QUEBRA DE LINHA MEU DEUS DUSA FD\n");
		tempLineIndex = -1;				/* If ch is a new line character, it prepare for the next line. */
		return '\n';				/* It returns a new line character. */
	}
	printf("Char returned is %c\n", ch);
	return ch;
}

Token peekNextToken()
{
	int i = 0;
	int num;
	KeyId cc;
	Token temp;
	char ident[MAXNAME];
	int tempSpaces = 0, tempCR = 0;
	while (1){				/* It counts the number of white spaces and new lines preceeding the next token. */
		if (ch == ' ')
			tempSpaces++;
		else if	(ch == '\t')
			tempSpaces+=TAB;
		else if (ch == '\n'){
			tempSpaces = 0;  tempCR++;
		}
		else break;
		ch = peekNextChar();
	}
	printf("WTF IS THIS %c\n", ch);
	switch (cc = charClassT[ch]) {
	case letter: 				/* identifier */
		do {
			if (i < MAXNAME)
				ident[i] = ch;
			i++; ch = peekNextChar();
		} while (  charClassT[ch] == letter
				|| charClassT[ch] == digit );
		if (i >= MAXNAME){
			errorMessage("too long");
			i = MAXNAME - 1;
		}	
		ident[i] = '\0'; 
		for (i=0; i<end_of_KeyWd; i++)
			if (strcmp(ident, KeyWdT[i].word) == 0) {
				temp.kind = KeyWdT[i].keyId;  		/* a reserved word */
				return temp;
			}
		temp.kind = Id;		/* A user defined name */
		strcpy(temp.u.id, ident);
		break;
	case digit: 					/* number */
		num = 0;
		do {
			num = 10*num+(ch-'0');
			i++; ch = peekNextChar();
		} while (charClassT[ch] == digit);
      		if (i>MAXNUM)
      			errorMessage("too large");
      		temp.kind = Num;
		temp.u.value = num;
		break;
	case colon:
		if ((ch = peekNextChar()) == '=') {
			ch = peekNextChar();
			temp.kind = Assign;		/*¡¡":="¡¡*/
			break;
		} else {
			temp.kind = nul;
			break;
		}
	case Lss:
		if ((ch = peekNextChar()) == '=') {
			ch = peekNextChar();
			temp.kind = LssEq;		/*¡¡"<="¡¡*/
			break;
		} else if (ch == '>') {
			ch = peekNextChar();
			temp.kind = NotEq;		/*¡¡"<>"¡¡*/
			break;
		} else {
			temp.kind = Lss;
			break;
		}
	case Gtr:
		if ((ch = peekNextChar()) == '=') {
			ch = peekNextChar();
			temp.kind = GtrEq;		/*¡¡">="¡¡*/
			break;
		} else {
			temp.kind = Gtr;
			break;
		}
	case Apostrophe:
		//FIXME First, the optimistic route
		ch = peekNextChar();
		temp.u.ch = ch;
		temp.kind = charString;
		if ((ch = peekNextChar()) != '\'') {
			//call error here
		}
		break;
	default:
		temp.kind = cc;
		ch = peekNextChar(); break;
	}
	printf("HEY THERE %s\n", ident);
	return temp;
}