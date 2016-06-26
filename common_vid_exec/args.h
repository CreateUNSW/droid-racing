#ifndef __OI_ARGS_H
#define __OI_ARGS_H

#include <string>
#include <vector>


enum ArgType
{
	AT_Flag,
	AT_Option,
};

class Args
{
private:
	std::vector<std::string>	argsshort;
	std::vector<std::string>	argslong;
	std::vector<std::string>	vals;
	std::vector<ArgType>		types;
	std::vector<std::string>	optionals;
	std::vector<std::string>	tabnames;

	std::vector<std::vector<std::string> >	options;
	std::vector<std::string>		defval;

	bool				bError;
	bool				bHelp;
	std::string			strArgError;
	std::string			strExe;
	std::string			strDescription;

public:
	Args();

	bool	argExists(const char *pchShort, const char *pchLong);
	bool	setFlag(const char *pchShort, const char *pchLong, bool optional = false);
	bool	setAlternateOption(const char *pchShort, const char *pchLong, const char *pchAlternate);
	bool	setOption(const char *pchShort, const char *pchLong, bool optional = false);
	bool	setOption(const char *pchShort, const char *pchLong, std::vector<std::string> options_, std::string defval_);
	bool	setOption(const char *pchShort, const char *pchLong, bool optional, std::string defval_);
	bool	setTabName(const char *pchShort, const char *pchTabName);
	void	setDescription(const char *pchDesc) { strDescription = pchDesc; }
	bool	handleArg(std::string &arg, std::vector<std::string> &list, int &i, int argc, char **argv);
	bool	parse(int argc, char **argv);
	bool	parse(char *pchCommandLine);
	bool	valid() { return !bError; }
	const	char *getError() const { return strArgError.c_str(); }
	bool	getFlag(const char *pchFlag);
	const	char *getOption(const char *pchOption);
	const	int getIntOption(const char *pchOption);
	int	getOptionIndex(const char *pchOtion);
	void	usage(bool bMessageBox = false);

	void	setVal(const char *pchShort, const char *pchVal);
};

#define MAX_OC_ARGS 60

#ifndef WIN32
void	InitArgs(char **ppchArgs);
bool	AddArg(char **ppchArgs, const char *pchArg, const char *pchVal, bool bVal, int &index);
bool	AddArg(char **ppchArgs, const char *pchArg, int iVal, bool bVal, int &index);
int	MyExeclp(const char *pch1, const char *pch2, char **ppchArgs);
#endif

#endif
