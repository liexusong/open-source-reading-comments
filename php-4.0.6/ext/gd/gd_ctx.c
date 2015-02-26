#define CTX_PUTC(c,ctx) ctx->putC(ctx, c)
	
static void _php_image_output_putc(struct gdIOCtx *ctx, int c)
{
	php_write(&c, 1);
}

static int _php_image_output_putbuf(struct gdIOCtx *ctx, const void* buf, int l)
{
	return php_write((void *)buf, l);
}

static void _php_image_output_ctxfree(struct gdIOCtx *ctx)
{
	if(ctx) {
		efree(ctx);
	}
}
	
static void _php_image_output_ctx(INTERNAL_FUNCTION_PARAMETERS, int image_type, char *tn, void (*func_p)()) 
{
	zval **imgind, **file, **quality;
	gdImagePtr im;
	char *fn = NULL;
	FILE *fp = NULL;
	int argc = ZEND_NUM_ARGS();
	int q = -1, i;
	gdIOCtx *ctx;
	GDLS_FETCH();

	/* The quality parameter for Wbmp stands for the threshold when called from image2wbmp() */
	
	if (argc < 1 || argc > 3 || zend_get_parameters_ex(argc, &imgind, &file, &quality) == FAILURE) 
	{
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(im, gdImagePtr, imgind, -1, "Image", le_gd);

	if (argc > 1) {
		convert_to_string_ex(file);
		fn = Z_STRVAL_PP(file);
		if (argc == 3) {
			convert_to_long_ex(quality);
			q = Z_LVAL_PP(quality);
		}
	}

	if ((argc == 2) || (argc == 3 && Z_STRLEN_PP(file))) {
		if (!fn || fn == empty_string || php_check_open_basedir(fn)) {
			php_error(E_WARNING, "%s: invalid filename '%s'", get_active_function_name(), fn);
			RETURN_FALSE;
		}

		fp = VCWD_FOPEN(fn, "wb");
		if (!fp) {
			php_error(E_WARNING, "%s: unable to open '%s' for writing", get_active_function_name(), fn);
			RETURN_FALSE;
		}

		ctx = gdNewFileCtx(fp);
	} else {
		ctx = emalloc(sizeof(gdIOCtx));
		ctx->putC = _php_image_output_putc;
		ctx->putBuf = _php_image_output_putbuf;
		ctx->free = _php_image_output_ctxfree;

#if APACHE && defined(CHARSET_EBCDIC)
		/* XXX this is unlikely to work any more thies@thieso.net */
		SLS_FETCH();
		/* This is a binary file already: avoid EBCDIC->ASCII conversion */
		ap_bsetflag(php3_rqst->connection->client, B_EBCDIC2ASCII, 0);
#endif
	}

	switch(image_type) {
		case PHP_GDIMG_CONVERT_WBM:
			if(q<0||q>255) {
				php_error(E_WARNING, "%s: invalid threshold value '%d'. It must be between 0 and 255",get_active_function_name(), q);
			}
		case PHP_GDIMG_TYPE_JPG:
			(*func_p)(im, ctx, q);
			break;
		case PHP_GDIMG_TYPE_WBM:
			for(i=0; i < gdImageColorsTotal(im); i++) {
				if(gdImageRed(im, i) == 0) break;
			} 
			(*func_p)(im, i, ctx);
			break;
		default:
			(*func_p)(im, ctx);
			break;
	}
	
	ctx->free(ctx);

	if(fp) {
		fflush(fp);
		fclose(fp);
	}
	
    RETURN_TRUE;
}
