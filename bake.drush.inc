<?php

function bake_drush_command() {
  return array(
    "bake" => array(
      'options' => array(
        'filter' => 'Include lines that are taged with this filters',
        'output' => 'The name of the ouput file',
        'stdin'  => ' a flag to pull the first file from stdin'
      ),
      'examples' => array(
        'drush bake basic.make custom.make' => ' Return merged make file of basic.make and custom.make',
        'drush bake basic.make custom.make --filter=dev' => ' Return merged make file of basic.make and custom.make including items tagged with dev',
        'cat basic.make | drush bake custom.make --stdin' => ' Return merged make file of basic.make and custom.make',
        'drush bake basic.make custom.make --output=finish.make' => ' place merged make file of basic.make and custom.make into finish.make',
      ),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    ),
  );
}

function bake_drush_help($section) {
  switch ($section) {
    case 'drush:bake':
      $help = <<<HELP
bake will combine two or more drush make files into one new file, with a few tricks.
Each file can delete line from previous files. This is done one of two ways 
1. ;del MATCH will find any line that has MATCH and remove it
2. ;del_r MATCH works the same way but MATCH is assume to be a regex pattern
also lines can be tag with a filter and only the lines that have the filter specified by --filter will be added to the end product. 
a like should start with ;#TAG# to tag a line with TAG
i line can be tage mulitip limes with ;#TAG1,TAG2#
if the --stdin flag is used the base file will come from stdin
if the --output=FILE the output will be put into the FILE file
HELP;
      return dt($help);
  }
}

function drush_bake() {
  /* 
   * parsing
   */
  $files = array();
  // if we are getting stdin put it in the files array first
  if (drush_get_option('stdin')) {
    $files['stdin'] = file("php://stdin");
  }
  $files_paths = func_get_args();
  foreach ($files_paths as $path) {
    $files[$path] = file($path);
  }
  /*
   * filtering
   */
  $filter_lines = array();

  // look into each file
  foreach($files as $path => $lines) {
    $lines_with_filters = preg_grep("|;#.*?#|", $lines);

    // look at each line that has a filter directive
    foreach ($lines_with_filters as $index =>$line) {
      preg_match("|;#(.*?)#|", $line, $line_filters_list);
      $line_filters = explode(',', $line_filters_list[1]);

      //foreach every filter on a line populate the filter_lines array
      foreach ($line_filters as $line_filter) {
        $filter_lines[$line_filter][$path.$index] = array(
          "path"=> $path,
          "index"=> $index,
          "line" => $line,
        );
      }
    }
  }
  // Take each filters lines and put them into include and exclude arrays
  $include_lines = array();
  $exclude_lines = array();
  $filters = array();
  if($filter_list = drush_get_option('filter')) {
    $filters = explode(",", $filter_list);
  }
  foreach ($filter_lines as $filter=> $lines) {
    if (in_array($filter, $filters)) {
    $include_lines = array_merge($include_lines, $lines);
    }
    else {
    $exclude_lines = array_merge($exclude_lines, $lines);
    }
  }
  // remove filter tokens for include
  foreach ($include_lines as $l) {
    $files[$l['path']][$l['index']] = preg_replace("|;#.*?# ?|", "", $l['line']);
  }
  // remove lines for exclude lines
  foreach ($exclude_lines as $l) {
    $files[$l['path']][$l['index']] = "REMOVED";
  }

  $output = array();
  foreach ($files as $path=> $lines) {
    /* 
     * deleting
     * find delete items
     * then remove them from the previous output
     */
    $del_token_pattern = "/;(del|del_r) /";
    $delete_token_lines = preg_grep($del_token_pattern, $lines);
    foreach ($delete_token_lines as $index => $line) {
      preg_match("/;(del|del_r) (.*)/", $line, $match);
      $pattern_type = $match[1];
      $pattern =$pattern_type == 'del_r' ? $match[2] : preg_quote($match[2]);
      $del_pattern = "`$pattern`";
      $lines[$index] = "REMOVED";
      $delete_output_lines = preg_grep($del_pattern, $output);
      foreach ($delete_output_lines as $output_index => $output_line) {
      $output[$output_index] = "REMOVED";
      }
    }
    // Merge lines into output
    $output = array_merge($output, $lines);
  }
  // remove empty lines
  $output = preg_grep("/^REMOVED$/", $output, PREG_GREP_INVERT);
  //convert $output to a string
  if ($output_file = drush_get_option('output')) {
    file_put_contents($output_file, $output);
  }
  else {
    print implode("", $output);
  }
}
