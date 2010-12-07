<?php
function bake_drush_command() {
  return array(
    "bake" => array(
      'options' => array(
        'type' => 'Pass on config info',
        'output' => 'The name of the ouput file',
        'stdin'  => ' a flag to pull the first file from stdin',
        'make' => 'The directory to make to after the file finish'
      ),
      'examples' => array(
        'drush bake basic.make custom.make' => ' Return merged make file of basic.make and custom.make',
        'drush bake basic.make custom.php --type=dev' => ' Return a make file started as basic.make and modified by custom.php',
        'cat basic.make | drush bake custom.make --stdin' => ' Return merged make file of basic.make and custom.make',
        'drush bake basic.make custom.make --output=finish.make' => ' place merged make file of basic.make and custom.make into finish.make',
        'drush bake basic.make custom.make --make=/sitepath' => ' merges make file of basic.make and custom.make and the use that to build a site at sitepath',
      ),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    ),
  );
}

function bake_drush_help($section) { 
  switch ($section) {
    case 'drush:bake':
      $help = <<<HELP
Drush bake is a drush command that takes make files and php includes and bake up a new drush make file.
it can have any number of arguments which can be .make files or .php files.  
If a .make files is passed it is parsed and merged with the previouse file
If a .php file is passed it is include, it has access to the following arrays
  \$info : which is the parsing of the previous make files
  \$config : which is populated with all of options
  the .php file can read and modify these to its hearts content.
  also as this ia a php file it has access to drush_get_option('OPTION') if it wants to use a undocument option.
HELP;
      return dt($help);
  }
}

function drush_bake() {
  $files = array();
  $stdinfile = FALSE;
  // if we are getting stdin put it in the files array first
  if (drush_get_option('stdin')) {
    $stdin = file_get_content("php://stdin");
      $stdinfile = tempname("/tmp", "stdin");
    if (preg_match("/<?php", $stdin)) {
      $stdinfile .= ".php";
    }
    else {
      $stdinfile .= ".make";
    }
    $files[] = $stinfile;
  }
  $args = func_get_args();
  $files = array_merge($files, $args);

  $info = array();
  $config = array(
    'type'=> drush_get_option('type'),
    'stdin'=> drush_get_option('stdin'),
    'make'=> drush_get_option('make'),
    'output'=> drush_get_option('output'),
  );

  foreach ($files as $file) {
    $details = pathinfo($file);
    if ($details['extension'] == 'php') {
      bake_include($info, $config, $file);
    }
    elseif ($details['extension'] == 'make') {
    $info = array_merge_recursive($info, drush_make_parse_info_file($file));
    }
  }
  $output = implode("\n",bake_unparse($info));
  if ($stdinfile) {
    unlink($stdinfile);
  }

  if ($out_file = drush_get_option('output')) {
    file_put_contents($out_file, $output);
  }
  elseif($make_dir = drush_get_option('make')) {
    $tmp_make_file = tempnam("/tmp", "make");
    file_put_contents($tmp_make_file, $output);
    system("drush make $tmp_make_file $make_dir");
    unlink($tmp_make_file);
  }
  else {
    print $output;
  }
}

function bake_unparse($array, $current = FALSE) {
  $lines = array();
  if (!$current) {
    foreach ($array as $key=>$value) {
      $lines = array_merge($lines, bake_unparse($value, $key));
    }
  }
  else {
    if(is_array($array)) {
      foreach ($array as $key=>$value) {
        $sub_current = $current;
        if (is_numeric($key)) {
          $sub_current .= '[]';
        }
        else {
          $sub_current .= "[$key]";
        }
        $lines = array_merge($lines, bake_unparse($value, $sub_current));
      }
    }
    else {
      $lines[] = "$current = $array";
    }
  }
  return $lines;
  
}

function bake_include(&$info, &$config, $file) {
  include($file);
}
