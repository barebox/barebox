#!/usr/bin/awk

/BAREBOX_CMD_HELP_START[[:space:]]*\((.*)\)/ {

	this_opt = 0;
	my_usage = "";
	my_short = "";
	my_cmd = gensub("BAREBOX_CMD_HELP_START[[:space:]]*\\((.*)\\)", "\\1", "g");
	this_text = 0;
	delete(my_text);
	delete(my_opts);
	next;
}

/BAREBOX_CMD_HELP_USAGE[[:space:]]*\((.*)\)/ {

	$0 = gensub("<", "\\&lt;", "g");
	$0 = gensub(">", "\\&gt;", "g");
	$0 = gensub("BAREBOX_CMD_HELP_USAGE[[:space:]]*\\((.*)\\)", "\\1", "g");
	$0 = gensub("\\\\n", "", "g");
	my_usage = gensub("\"", "", "g");
	next;

}

/BAREBOX_CMD_HELP_SHORT[[:space:]]*\((.*)\)/ {

	$0 = gensub("<", "\\&lt;", "g");
	$0 = gensub(">", "\\&gt;", "g");
	$0 = gensub("BAREBOX_CMD_HELP_SHORT[[:space:]]*\\((.*)\\)", "\\1", "g");
	$0 = gensub("\\\\n", "", "g");
	my_short = gensub("\"", "", "g");
	next;

}

/BAREBOX_CMD_HELP_OPT[[:space:]]*\([[:space:]]*(.*)[[:space:]]*,[[:space:]]*(.*)[[:space:]]*\)/ {

	$0 = gensub("<", "\\&lt;", "g");
	$0 = gensub(">", "\\&gt;", "g");
	$0 = gensub("@", "\\\\@",    "g");	
	$0 = gensub("BAREBOX_CMD_HELP_OPT[[:space:]]*\\([[:space:]]*\"*(.*)\"[[:space:]]*,[[:space:]]*\"(.*)\"[[:space:]]*\\)", \
		"<tr><td><tt> \\1 </tt></td><td>\\&nbsp;\\&nbsp;\\&nbsp;</td><td> \\2 </td></tr>", "g");
	$0 = gensub("\\\\n", "", "g");
	my_opts[this_opt] = gensub("\"", "", "g");
	this_opt ++;
	next;
}

/BAREBOX_CMD_HELP_TEXT[[:space:]]*\((.*)\)/ {

	$0 = gensub("<", "\\&lt;", "g");
	$0 = gensub(">", "\\&gt;", "g");
	$0 = gensub("BAREBOX_CMD_HELP_TEXT[[:space:]]*\\((.*)\\)", "\\1", "g");
	$0 = gensub("\\\\n", "<br>", "g");
	my_text[this_text] = gensub("\"", "", "g");
	this_text ++;
	next;
}

/BAREBOX_CMD_HELP_END/ {

	printf "/**\n";
	printf " * @page " my_cmd "_command " my_cmd "\n";
	printf " *\n";
	printf " * \\par Usage:\n";
	printf " * " my_usage "\n";
	printf " *\n";

	if (this_opt != 0) {
		printf " * \\par Options:\n";
		printf " *\n";
		printf " * <table border=\"0\" cellpadding=\"0\">\n";
		n = asorti(my_opts, my_opts_sorted);
		for (i=1; i<=n; i++) {
			printf " * " my_opts[my_opts_sorted[i]] "\n";
		}
		printf " * </table>\n";
		printf " *\n";
	}

	printf " * " my_short "\n";
	printf " *\n";

	n = asorti(my_text, my_text_sorted);
	if (n > 0) {
		for (i=1; i<=n; i++) {
			printf " * " my_text[my_text_sorted[i]] "\n";
		}
		printf " *\n";
	}

	printf " */\n";

	next;
}

/^.*$/ {

	print $0;

}

