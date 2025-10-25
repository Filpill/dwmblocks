//Modify this file to change what commands output to your statusbar, and recompile using the make command.
static const Block blocks[] = {
	/*Icon*/	/*Command*/		/*Update Interval*/	/*Update Signal*/  /*Click Command*/
  {" ", "nmcli -t -f active,ssid dev wifi | grep '^yes' | cut -d: -f2",			60,			1, "st -e nmtui"},

  {"󰕾 ", "pactl get-sink-volume \"$(pactl get-default-sink)\" | awk -F'/' '/Volume:/{gsub(/ /,\"\"); print $2; exit}'", 30, 2, "st -e pulsemixer" },

	{"󰍛 ", "free -h | awk '/^Mem/ { print $3\"/\"$2 }' | sed s/i//g",	30,		3, "st -e htop"},

  {" ", "[ -f /sys/class/power_supply/BAT1/capacity ] && cat /sys/class/power_supply/BAT1/capacity | awk '{print $1\"%\"}'",	60,			4, "st -e htop"},

	{"󰃭 ", "date '+%b %d'",					5,		5, "gsimplecal"},

	{" ", "date '+%H:%M'",					5,		6, "gsimplecal"},

};

//sets delimiter between status commands. NULL character ('\0') means no delimiter.
static char delim[] = " | ";
static unsigned int delimLen = 3;
