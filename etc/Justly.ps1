$bin_dir = Split-Path $MyInvocation.MyCommand.Path
$prefix = Split-Path $bin_dir
$OPCODE6DIR64 = Join-Path $prefix "plugins"
& "$bin_dir/Justly" $args