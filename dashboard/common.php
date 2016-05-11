<?php

function give_error($msg) {
  $error_message = $msg;
  require("ui/error.php");
}

class PluginStatus {
  public $name;
  public $lastcheck;
  public $state;
  public $latency;
  public $errcode;
  public $explanation;

  function __construct($str) {
      $info = explode("%%", $str);

      $this->name = $info[1];
      $this->lastcheck = $info[2];
      $this->state = $info[3];
      $this->latency = $info[4];
      $this->errcode = $info[5];
      $this->explanation = $info[6];
  }
}

class AllPluginStatus {
  public $plugins = array();

  function __construct($str) {
    $plugins = explode("&&", $str);
    foreach ($plugins as $plugin) {
      $pluginStatus = new PluginStatus($plugin);
      $this->plugins[$pluginStatus->name] = $pluginStatus;
    }
  }
}

class AvailabilityChunk {
  public $start;
  public $end;
  public $status;

  public $normalized;

  function __construct($start, $end, $status) {
    $this->start = $start;
    $this->end = $end;
    $this->status = $status;
  }
}

class AvailabilityStats {
  public $chunks = array();

  function addChunk($chunk) {
    // sometimes dynafed will report lastcheck as 0 unix time.. ignore those data points
    if($chunk->start <= $chunk->end && $chunk->start > 0 && $chunk->end > 0) {
      $this->chunks[] = $chunk;
    }
  }

  function __construct($history, $target) {
    $start = $history[0]->plugins[$target]->lastcheck;
    $status = $history[0]->plugins[$target];

    foreach($history as $h) {
      if($h->plugins[$target]->state != $status->state) {
        $this->addChunk(new AvailabilityChunk($start, $h->plugins[$target]->lastcheck, $status));

        $start = $h->plugins[$target]->lastcheck;
        $status = $h->plugins[$target];
      }
    }
    $this->addChunk(new AvailabilityChunk($start, end($history)->plugins[$target]->lastcheck, $status));

    $totalDuration = 0;
    foreach($this->chunks as $chunk) {
      $totalDuration += $chunk->end - $chunk->start - 1;
    }

    foreach($this->chunks as $key => $chunk) {
      $normalized = ($chunk->end - $chunk->start - 1) / $totalDuration;
      if($normalized < 0) {
        $normalized = 0;
      }
      $this->chunks[$key]->normalized = $normalized;
    }
  }
}

?>
