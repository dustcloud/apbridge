#
# Voyager APC shell environment and alias setup
#

# Environment variables
export APC_HOME=/opt/dust-apc
export PATH=${APC_HOME}/bin:${PATH}
export LD_LIBRARY_PATH=$APC_HOME/lib:${LD_LIBRARY_PATH}

alias apcctl='$APC_HOME/bin/apcctl'
alias apc-console='$APC_HOME/bin/apc_console.py'
alias update-apc-config='$APC_HOME/bin/update-apc-config.py'


