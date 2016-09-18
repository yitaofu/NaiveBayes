

// 编码，解码-----------------------------
static char hex2dec(char ch){
	if (ch >= 'A' && ch <= 'Z'){
		ch = (ch -'A') + 'a';
	} 
	return (ch >= '0' && ch <= '9') ? (ch - '0') : (ch - 'a' + 10);
}

static char dec2hex(char ch){
	static char hex[] = "0123456789ABCDEF";
	return hex[ch & 15];
}

// urlencode 编码
char *urlencode(char *src){
	char *psrc = src;
	char *buf;
	char *pbuf;
	int len = strlen(src) * 3 + 1;
	buf = (char *)malloc(len);
	// errdo(buf == NULL, src);
	memset(buf, 0, len);

	pbuf = buf;
	while (*psrc){
		if ((*psrc >= '0' && *psrc <= '9') || (*psrc >= 'a' && *psrc <= 'z') || (*psrc >= 'A' && *psrc <= 'Z')){
			*pbuf = *psrc;
		}else if (*psrc == '-' || *psrc == '_' || *psrc == '.' || *psrc == '~'){
			*pbuf = *psrc;
		}else if (*psrc == ' '){
			*pbuf = '+';
		}else{
			pbuf[0] = '%';
			pbuf[1] = dec2hex(*psrc >> 4);
			pbuf[2] = dec2hex(*psrc & 15);
			pbuf += 2;
		}
		
		psrc++;
		pbuf++;
	}

	*pbuf = '\0';
	return buf;
}

// urldecode 解码
char *urldecode(char *src){
	char *psrc = src;
	char *buf;
	char *pbuf;
	int len = strlen(src) + 1;
	buf = (char *)malloc(len);
	// errdo(buf == NULL, src);
	memset(buf, 0, len);

	pbuf = buf;
	while (*psrc){
		if (*psrc == '%'){
			if (psrc[1] && psrc[2]){
				*pbuf = hex2dec(psrc[1]) << 4 | hex2dec(psrc[2]);
				psrc += 2;
			}
		}else if (*psrc == '+'){
			*pbuf = ' ';
		}else{
			*pbuf = *psrc;
		}

		psrc++;
		pbuf++;
	}

	*pbuf = '\0';
	return buf;
}
// --------------------------------------------
