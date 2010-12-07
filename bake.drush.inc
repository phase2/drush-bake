<?php

/*
 * implimentation of hook_drush_command()
 */
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

/*
 * implimentation of hook_drush_help
 */
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

/*
 * the command for dursh bake
 */
function drush_bake() {
  $files = array();
  $stdinfile = FALSE;
  // if we are getting stdin put in a temp file
  // and then add that file to the files array
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
  
  //grab args and add them to files
  $args = func_get_args();
  $files = array_merge($files, $args);

  //init the info and config arrays
  $info = array();
  $config = array(
    'type'=> drush_get_option('type'),
    'stdin'=> drush_get_option('stdin'),
    'make'=> drush_get_option('make'),
    'output'=> drush_get_option('output'),
  );

  foreach ($files as $file) {
    if (is_file($file)) {
      $details = pathinfo($file);
      if ($details['extension'] == 'php') {
        //if a php file then let it modify info and config
        bake_include($info, $config, $file);
      }
      elseif ($details['extension'] == 'make') {
        //if a make file merge it to the current info array
        //TODO: check to insure we have a parseable file
      $info = array_merge_recursive($info, drush_make_parse_info_file($file));
      }
    }
  }
  
  //if we created temp stdinfile remove it
  if ($stdinfile) {
    unlink($stdinfile);
  }

  // BUILD OUTPUT
  $output = implode("\n",bake_unparse($info));
  if ($out_file = drush_get_option('output')) {
    file_put_contents($out_file, $output);
  }
  elseif($make_dir = drush_get_option('make')) {
    $tmp_make_file = tempnam("/tmp", "make");
    file_put_contents($tmp_make_file, $output);
    //TODO: I am sure there is a drush way to do this but
    //I do not know what it is
    system("drush make $tmp_make_file $make_dir");
    unlink($tmp_make_file);
  }
  else {
    print $output;
  }
}

/*
 * takes an array and returns an array of lines formating
 * like a php info file
 * this is a recursive function and should only be called with
 * $array populated
 */
function bake_unparse($array, $current = FALSE) {
  $lines = array();
  if (!$current) {
    // if this is the first pass then we can use the key as the current
    foreach ($array as $key=>$value) {
      $lines = array_merge($lines, bake_unparse($value, $key));
    }
  }
  else {
    if(is_array($array)) {
      foreach ($array as $key=>$value) {
        $sub_current = $current;
        //$the key is numeric so we write the new item []
        //to current before passing off to bake_unparse
        if (is_numeric($key)) {
          $sub_current .= '[]';
        }
        // the $key is not numeric so we add the hash look up [$key]
        // before anding off to bake_unparse
        else {
          $sub_current .= "[$key]";
        }
        $lines = array_merge($lines, bake_unparse($value, $sub_current));
      }
    }
    else {
      //$array is a value so now it is time to write the assignemt
      $lines[] = "$current = $array";
    }
  }
  return $lines;
  
}

/*
 * includes the $file, sandbox variable scoop to $info and $config
 */
function bake_include(&$info, &$config, $file) {
  include($file);
}
