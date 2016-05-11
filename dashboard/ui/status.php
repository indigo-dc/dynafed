<h2 class="sub-header">Current status</h2>

<p class="lead">Showing poll #<?php echo $idx ?></p>

<div class="table-responsive">
  <table class="table table-hover">
    <thead>
      <tr>
        <td>State</td>
        <td>Name</td>
        <td>Last check</td>
        <td>Latency (ms)</td>
      </tr>
    </thead>
    <tbody>
<?php
    foreach ($currentStatus->plugins as $point) {

      $td_class = "success";
      $rowspan = 1;

      if($point->state != 1) {
        $td_class = "danger";
        $rowspan = 2;
      }
?>

      <tr>
        <td rowspan="<?php echo $rowspan ?>" class="<?php echo $td_class ?>"><?php echo $point->state ?></td>
        <td><?php echo $point->name ?></td>
        <td><?php echo gmdate("h:m:s Y-m-d", $point->lastcheck) ?></td>
        <td><?php echo $point->latency ?></td>
      </tr>

<?php if($point->state != 1) { ?>
      <tr>
        <td colspan="3"><?php echo "($point->errcode) $point->explanation" ?></td>
      </tr>


<?php }
    } ?>

    </tbody>
  </table>
</div>
