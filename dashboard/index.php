<?php

require("config.php");
require("common.php");

require("ui/header.php");

$mem = new Memcache();
$ret = $mem->connect($memhost, $memport);

if(!$ret) {
  give_error("Unable to establish a connection to the memcache daemon on $memhost:$memport. Please check config.php.");
}
else {
  $idx = $mem->get("Ugrpluginstats_idx") - 1;
  $current = $mem->get("Ugrpluginstats_$idx");
  $currentStatus = new AllPluginStatus($current);
  require("ui/status.php");

  // load all previous history
  $history = array();
  for ($i=$idx; $i >= $idx-1000+1; $i--) {
    $str = $mem->get("Ugrpluginstats_$i");
    if($str == "") break;
    $history[] = new AllPluginStatus($mem->get("Ugrpluginstats_$i"));
  }
  $history = array_reverse($history);
  require("ui/history.php");
}

require("ui/footer.php");
?>
