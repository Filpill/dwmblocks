//Modify this file to change what commands output to your statusbar, and recompile using the make command.
static const Block blocks[] = {
	/*Icon*/	/*Command*/		/*Update Interval*/	/*Update Signal*/
  {"  ", "nmcli -t -f active,ssid dev wifi | grep '^yes' | cut -d: -f2",			60,			0},

  {"  ", "cat /sys/class/power_supply/BAT1/capacity | awk '{print $1\"%\"}'",	60,			0},

	{"󰄨 ", "free -h | awk '/^Mem/ { print $3\"/\"$2 }' | sed s/i//g",	30,		0},

	{" ", "date '+%Y %b %d (%a) |  %H:%M'",					5,		0},
};

//sets delimiter between status commands. NULL character ('\0') means no delimiter.
static char delim[] = " | ";
static unsigned int delimLen = 5;
