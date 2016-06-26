#include "args.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syslog.h>

Args::Args() :
	bError(false)
	,bHelp(false)
{
	setFlag("h", "help", true);
}

bool
Args::argExists(const char *pchShort, const char *pchLong)
{
	bool exists = false;
	int i;
	std::string strShort = pchShort;
	std::string strLong  = pchLong;

	for (i=0;i<(int)argsshort.size();i++)
	{
		if (argsshort[i] == strShort)
		{
			exists = true;
			break;
		}
	}

	if (!exists)
	{
		for (i=0;i<(int)argslong.size();i++)
		{
			if (argslong[i] == strLong)
			{
				exists = true;
				break;
			}
		}
	}

	if (exists)
	{
		bError = true;
		strArgError = "argument ";
		strArgError += pchShort;
		strArgError += " already exists";
	}
	return exists;
}

bool
Args::setFlag(const char *pchShort, const char *pchLong, bool optional)
{
	std::vector<std::string> svO;
	std::string dv;

	if (argExists(pchShort, pchLong))
		return false;

	argsshort.push_back(pchShort);
	argslong.push_back(pchLong);
	optionals.push_back(optional ? "true" : "false");
	types.push_back(AT_Flag);
	vals.push_back("");
	tabnames.push_back("");

	options.push_back(svO);
	defval.push_back(dv);
	return true;
}


//in this case either one argument or the other must be provided
bool
Args::setAlternateOption(const char *pchShort, const char *pchLong, const char *pchAlternate)
{
	std::vector<std::string> svO;
	std::string dv;

	if (argExists(pchShort, pchLong))
		return false;
	argsshort.push_back(pchShort);
	argslong.push_back(pchLong);
	optionals.push_back(pchAlternate);
	types.push_back(AT_Option);
	vals.push_back("");
	tabnames.push_back("");

	options.push_back(svO);
	defval.push_back(dv);
	return true;
}


bool
Args::setOption(const char *pchShort, const char *pchLong, bool optional)
{
	std::vector<std::string> svO;
	std::string dv;

	if (argExists(pchShort, pchLong))
		return false;
	argsshort.push_back(pchShort);
	argslong.push_back(pchLong);
	optionals.push_back(optional ? "true" : "false");
	types.push_back(AT_Option);
	vals.push_back("");
	tabnames.push_back("");

	options.push_back(svO);
	defval.push_back(dv);
	return true;
}

bool
Args::setOption(const char *pchShort, const char *pchLong, bool optional, std::string defval_)
{
	std::vector<std::string> svO;

	if (argExists(pchShort, pchLong))
		return false;
	argsshort.push_back(pchShort);
	argslong.push_back(pchLong);
	optionals.push_back(optional ? "true" : "false");
	types.push_back(AT_Option);
	vals.push_back("");
	tabnames.push_back("");

	options.push_back(svO);
	defval.push_back(defval_);
	return true;
}

bool
Args::setOption(const char *pchShort, const char *pchLong, std::vector<std::string> options_, std::string defval_)
{
	if (argExists(pchShort, pchLong))
		return false;
	argsshort.push_back(pchShort);
	argslong.push_back(pchLong);
	optionals.push_back("false");
	types.push_back(AT_Option);
	vals.push_back("");
	tabnames.push_back("");

	options.push_back(options_);
	defval.push_back(defval_);
	return true;
}

bool
Args::setTabName(const char *pchShort, const char *pchTabName)
{
	size_t i;
	for(i=0;i<argsshort.size();i++)
	{
		if (!CaseInsensitiveCompare(argsshort[i].c_str(), pchShort))
		{
			tabnames[i] = pchTabName;
			return true;
		}
	}
	return false;
}

bool
Args::handleArg(std::string &arg, std::vector<std::string> &list, int &i, int argc, char **argv)
{
	int	j;
	bool	found = false;
	bool	err   = false;

	for(j=0;j<(int)list.size();j++)
	{
		if (arg == list[j])
		{
			found = true;
			break;
		}
	}

	if (found)
	{
		if (types[j] == AT_Flag)
		{
			vals[j] = "true";
		}
		else
		{
			if (i < (argc-1))
			{
				i++;
				vals[j] = argv[i];
			}
			else
			{
				strArgError = "option value missing for option ";
				strArgError += list[j];
				err = true;
			}
		}
	}
	else
	{
		strArgError = "short argument ";
		strArgError += arg;
		strArgError += " is invalid";
		err = true;
	}

	return err;
}

bool
Args::parse(char *pchCommandLine)
{
	char *p = pchCommandLine;
	bool bInQuote = false;
	int iArgs = 0, i;

	char achArg[256];
	char **args = new char *[MAX_OC_ARGS];
	for(i=0;i<MAX_OC_ARGS;i++)
	{
		args[i] = new char[256];
	}

	strcpy(args[iArgs], "dummy");
	iArgs++;

	memset(achArg, 0, sizeof(achArg));
	char *q = achArg;

	while (p && *p)
	{
		if (!bInQuote)
		{
			if (*p == ' ')
			{
				*q = 0;
				strcpy(args[iArgs], achArg);
				iArgs++;
				memset(achArg, 0, sizeof(achArg));
				q = achArg;
			}
			else if (*p == '"' || *p == '\'')
			{
				bInQuote = true;
			}
			else
			{
				*q++ = *p;
			}
		}
		else
		{
			if (*p == '"' || *p == '\'')
			{
				bInQuote = false;
			}
			else
			{
				*q++ = *p;
			}
		}
		if (iArgs >= MAX_OC_ARGS)
			break;
		p++;
	}
	*q = 0;
	strcpy(args[iArgs], achArg);
	iArgs++;

	bool bRet = parse(iArgs, args);
	for(i=0;i<MAX_OC_ARGS;i++)
	{
		delete [] args[i];
	}
	delete [] args;
	return bRet;
}

bool
Args::parse(int argc, char **argv)
{
	int i;
	strExe = argv[0];
	for (i=1;i<argc;i++)
	{
		std::string strArgLong;
		std::string strArgShort;
		char *pch = argv[i];

		if (pch && *pch)
		{
			if (!CaseInsensitiveCompare(pch, "--", 2))
			{
				strArgLong = pch+2;
			}
			else if (!CaseInsensitiveCompare(pch, "-", 1))
			{
				strArgShort = pch+1;
			}

			if (!strArgShort.empty())
			{
				bError = handleArg(strArgShort, argsshort, i, argc, argv);
			}
			else if (!strArgLong.empty())
			{
				bError = handleArg(strArgLong, argslong, i, argc, argv);
			}
			else
			{
				strArgError = "invalid argument format for ";
				strArgError += pch;
				bError = true;
			}
		}
		if (bError)
			break;
	}

	if (!bError)
	{
		for (i=0;i<(int)optionals.size();i++)
		{
			if (argslong[i] == "help" && vals[i] == "true")
			{
				bHelp = true;
				bError = true;
			}
		}
	}

	if (!bError && !bHelp)
	{
		for (i=0;i<(int)optionals.size();i++)
		{
			if (optionals[i] == "false")
			{
				if (options[i].size())
				{
					if (vals[i].empty())
					{
						vals[i] = defval[i];
					}
					else
					{
						int j;
						bool valid = false;
						for(j=0;j<(int)options[i].size();j++)
						{
							if (vals[i] == options[i][j])
							{
								valid = true;
								break;
							}
							if (options[i][j] == "")
							{
								valid = true;
								break;
							}
						}
						if (!valid)
						{
							strArgError = "invalid argument '";
							strArgError += argsshort[i];
							strArgError += "' '";
							strArgError += argslong[i];
							strArgError += "' ";
							bError = true;
							break;
						}
					}
				}
				else
				{
					if (vals[i].empty())
					{
						if (!defval[i].empty())
						{
							vals[i] = defval[i];
						}
						else
						{
							strArgError = "mandatory argument '";
							strArgError += argsshort[i];
							strArgError += "' '";
							strArgError += argslong[i];
							strArgError += "' not supplied";
							bError = true;
							break;
						}
					}
				}
			}
			else if (optionals[i] == "true")
			{
				if (vals[i].empty())
				{
					if (!defval[i].empty())
					{
						vals[i] = defval[i];
					}
				}
			}
			else //we have an alternative for a mandatory option
			{
				if (vals[i].empty())
				{
					//our val is empty make sure the alternative option's val is not empty
					int j;
					for (j=0;j<(int)argsshort.size();j++)
					{
						if (argsshort[j] == optionals[i])
						{
							if (vals[j].empty())
							{
								strArgError = "alternative arguments '";
								strArgError += argsshort[i];
								strArgError += "' and '";
								strArgError += argsshort[j];
								strArgError += "' neither supplied";
								bError = true;
								break;
							}
						}
					}
				}
			}
		}
	}

	return !bError;
}

void
Args::setVal(const char *pchShort, const char *pchVal)
{
	std::string strArgShort = pchShort;

	if (!strArgShort.empty())
	{
		int j;
		for(j=0;j<(int)argsshort.size();j++)
		{
			if (strArgShort == argsshort[j])
			{
				vals[j] = pchVal;
				break;
			}
		}
	}
}

bool
Args::getFlag(const char *pchFlag)
{
	bool bSet = false;

	if (pchFlag)
	{
		int i;
		for(i=0;i<(int)argslong.size();i++)
		{
			if (argslong[i] == pchFlag)
			{
				if (!CaseInsensitiveCompare(vals[i].c_str(), "true"))
				{
					bSet = true;
					break;
				}
			}
		}

		if (!bSet)
		{
			int i;
			for(i=0;i<(int)argsshort.size();i++)
			{
				if (argsshort[i] == pchFlag)
				{
					if (!CaseInsensitiveCompare(vals[i].c_str(), "true"))
					{
						bSet = true;
						break;
					}
				}
			}
		}
	}

	return bSet;
}

const	char *
Args::getOption(const char *pchOption)
{
	bool	bSet = false;
	bool	bFound = false;
	const char *opt = 0;

	if (pchOption)
	{
		int i;
		for(i=0;i<(int)argslong.size();i++)
		{
			if (argslong[i] == pchOption)
			{
				bFound = true;
				if (!vals[i].empty())
				{
					bSet = true;
					opt = vals[i].c_str();
					break;
				}
			}
		}

		if (!bSet)
		{
			int i;
			for(i=0;i<(int)argsshort.size();i++)
			{
				if (argsshort[i] == pchOption)
				{
					bFound = true;
					if (!vals[i].empty())
					{
						bSet = true;
						opt = vals[i].c_str();
						break;
					}
				}
			}
		}
		if (!bFound)
		{
			printf("Invalid argument '%s' requested in getOption()\n", pchOption);
		}
	}
	return opt;
}

const	int
Args::getIntOption(const char *pchOption)
{
	if (getOption(pchOption))
		return atoi(getOption(pchOption));
	return -1;
}

int
Args::getOptionIndex(const char *pchOption)
{
	bool	bSet = false;
	int	opt = 0;

	if (pchOption)
	{
		int i;
		for(i=0;i<(int)argslong.size();i++)
		{
			if (argslong[i] == pchOption)
			{
				if (!vals[i].empty())
				{
					if (options[i].size())
					{
						int j;
						for(j=0;j<(int)options[i].size();j++)
						{
							if (options[i][j] == vals[i])
							{
								bSet = true;
								opt = j;
								break;
							}
						}
					}
				}
			}
		}

		if (!bSet)
		{
			int i;
			for(i=0;i<(int)argsshort.size();i++)
			{
				if (argsshort[i] == pchOption)
				{
					if (options[i].size())
					{
						int j;
						for(j=0;j<(int)options[i].size();j++)
						{
							if (options[i][j] == vals[i])
							{
								bSet = true;
								opt = j;
								break;
							}
						}
					}
				}
			}
		}
	}

	return opt;
}


void
Args::usage(bool bMessageBox)
{
	std::string strMsg;
	char achTemp[1024];

#ifndef WIN32
	bMessageBox = false;
#endif
	if (bHelp)
	{
		sprintf(achTemp, "Usage for %s\n", strExe.c_str());
		strMsg += achTemp;
		if (!strDescription.empty())
		{
			sprintf(achTemp, "Description : %s\n", strDescription.c_str());
			strMsg += achTemp;
		}

		int i;
		for (i=0;i<(int)optionals.size();i++)
		{
			char achDef[512];
			*achDef = 0;

			char achTab[256];
			*achTab = 0;

			if (!tabnames[i].empty())
				sprintf(achTab, " : tabname %s", tabnames[i].c_str());

			if (types[i] == AT_Flag)
			{
				if (!defval[i].empty())
					sprintf(achDef, " : default %s", defval[i].c_str());

				if (optionals[i] == "false")
					sprintf(achTemp, " -%s --%s : mandatory argument%s%s\n", argsshort[i].c_str(), argslong[i].c_str(), achDef, achTab);
				else
					sprintf(achTemp, " -%s --%s%s%s\n", argsshort[i].c_str(), argslong[i].c_str(), achDef, achTab);
				strMsg += achTemp;
			}
			else
			{
				if (optionals[i] == "false")
				{
					if (options[i].size())
					{
						sprintf(achTemp, " -%s val --%s val : options ", argsshort[i].c_str(), argslong[i].c_str());
						strMsg += achTemp;
						int j;
						for(j=0;j<(int)options[i].size();j++)
						{
							bool bDef = options[i][j] == defval[i];
							sprintf(achTemp, "%s%s%s%s", j == 0 ? "" : ",", bDef ? "[" : "", options[i][j].c_str(), bDef ? "]" : "");
							strMsg += achTemp;
						}
						sprintf(achTemp, "%s%s\n", "", achTab);
						strMsg += achTemp;
					}
					else
					{
						if (!defval[i].empty())
							sprintf(achDef, " : default %s", defval[i].c_str());
						sprintf(achTemp, " -%s val --%s val : mandatory argument%s%s\n", argsshort[i].c_str(), argslong[i].c_str(), achDef, achTab);
						strMsg += achTemp;
					}
				}
				else
				{
					if (!defval[i].empty())
						sprintf(achDef, " : default %s", defval[i].c_str());
					sprintf(achTemp, " -%s val --%s val%s%s\n", argsshort[i].c_str(), argslong[i].c_str(), achDef, achTab);
					strMsg += achTemp;
				}
			}
		}
		if (bMessageBox)
		{
#ifdef WIN32
			MessageBox(0, strMsg.c_str(), "Usage", 0);
#endif
		}
		else
		{
			printf("%s" ,strMsg.c_str());
		}
	}
	else
	{
		if (bMessageBox)
		{
#ifdef WIN32
			sprintf(achTemp, "%s\n", strArgError.c_str());
			MessageBox(0, achTemp, "Error", 0);
#endif
		}
		else
		{
			fprintf(stderr, "%s\n", strArgError.c_str());
		}
#ifndef WIN32
		syslog(LOG_ERR, "Invalid arguments %s\n", strArgError.c_str());
#endif
	}
}

#ifndef WIN32
void
InitArgs(char **ppchArgs)
{
	int i;
	for(i=0;i<MAX_OC_ARGS;i++)
		ppchArgs[i] = 0;
}

bool
AddArg(char **ppchArgs, const char *pchArg, const char *pchVal, bool bVal, int &index)
{
	if (bVal)
	{
		if (index > (MAX_OC_ARGS - 2))
			return false;
		if (pchArg && *pchArg && pchVal && *pchVal)
		{
			ppchArgs[index++] = StringDup(pchArg);
			ppchArgs[index++] = StringDup(pchVal);
		}
	}
	else
	{
		if (index > (MAX_OC_ARGS - 1))
			return false;
		ppchArgs[index++] = StringDup(pchArg);
	}
	return true;
}

bool
AddArg(char **ppchArgs, const char *pchArg, int iVal, bool bVal, int &index)
{
	char ach[32];
	sprintf(ach, "%d", iVal);
	bool bRet = AddArg(ppchArgs, pchArg, ach, bVal, index);
	return bRet;
}

int
MyExeclp(const char *pch1, const char *pch2, char **ppchArg)
{
	int x = execlp(pch1, pch2,
	       		ppchArg[0],
			ppchArg[1],
			ppchArg[2],
			ppchArg[3],
			ppchArg[4],
			ppchArg[5],
			ppchArg[6],
			ppchArg[7],
			ppchArg[8],
			ppchArg[9],
	       		ppchArg[10],
			ppchArg[11],
			ppchArg[12],
			ppchArg[13],
			ppchArg[14],
			ppchArg[15],
			ppchArg[16],
			ppchArg[17],
			ppchArg[18],
			ppchArg[19],
	       		ppchArg[20],
			ppchArg[21],
			ppchArg[22],
			ppchArg[23],
			ppchArg[24],
			ppchArg[25],
			ppchArg[26],
			ppchArg[27],
			ppchArg[28],
			ppchArg[29],
	       		ppchArg[30],
			ppchArg[31],
			ppchArg[32],
			ppchArg[33],
			ppchArg[34],
			ppchArg[35],
			ppchArg[36],
			ppchArg[37],
			ppchArg[38],
			ppchArg[39],
	       		ppchArg[40],
			ppchArg[41],
			ppchArg[42],
			ppchArg[43],
			ppchArg[44],
			ppchArg[45],
			ppchArg[46],
			ppchArg[47],
			ppchArg[48],
			ppchArg[49],
	       		ppchArg[50],
			ppchArg[51],
			ppchArg[52],
			ppchArg[53],
			ppchArg[54],
			ppchArg[55],
			ppchArg[56],
			ppchArg[57],
			ppchArg[58],
			ppchArg[59],
	       		NULL);
	int i;
	for(i=0;i<MAX_OC_ARGS;i++)
	{
		if (ppchArg[i])
			delete [] ppchArg[i];
	}
	return x;
}

#endif
