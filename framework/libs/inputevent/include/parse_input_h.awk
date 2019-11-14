BEGIN {
	print("#include <linux/input.h>");
}
/Keys and buttons/ { 
	print("\nstatic const char *KEY_NAME[KEY_CNT] = {"); 
	next;
}

/Switch events/ { 
	print("\nstatic const char *SW_NAME[SW_CNT] = {"); 
	next;
}

/#define (SW|BTN|KEY)_\w/ {
	if($2 ~ /(KEY|SW)_(MIN_INTERESTING|MAX)/) {
		next;
	} else if($2 ~ /(KEY|SW)_CNT/) {
		print("};");
		next;
	}	

	if(substr($2, 0, 3) ~ /(KEY|SW)/) {
		name=substr($2, index($2, "_")+1); 
	} else {
		name=$2;
	}

	printf("%4s[%-25s] = \"%s\",\n", "", $2, name);
}


