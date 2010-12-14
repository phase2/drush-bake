<?php

/*
 * implimentation of hook_drush_command()
 */
function bake_drush_command() { 
  return array(
    "bake" => array(
      'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
      'option' => array(
        'type' => 'Pass on config info',
      ),
      'examples' => array(
        'drush bake A.bake' => "Creates a A.make file and then build a site i A/ with that file",
        'drush bake A.bake B' => "Creates a A.make file and then builds a site in B/ with that file",
      ),
    ) ,
    "bakeit" => array(
      'options' => array(
        'type' => 'Pass on config info',
        'output' => 'The name of the ouput file',
        'stdin'  => ' a flag to pull the first file from stdin',
        'make' => 'The directory to make to after the file finish',
        'version' => 'FOR MAKE Print the version and exit.',
        'contrib-destination=path' => 'FOR MAKE Specify a path under which modules and themes should be placed. Defaults to sites/all.',
        'force-complete' => 'FOR MAKE Force a complete build even if errors occur.',
        'md5' => 'FOR MAKE Output an md5 hash of the current build after completion.',
        'no-clean' => 'FOR MAKE Leave temporary build directories in place instead of cleaning up after completion.',
        'no-core' => 'FOR MAKE Do not require a Drupal core project to be specified.',
        'no-patch-txt' => 'FOR MAKE Do not write a PATCHES.txt file in the directory of each patched project.',
        'prepare-install' => 'FOR MAKE Prepare the built site for installation. Generate a properly permissioned settings.php and files directory.',
        'tar' => 'FOR MAKE Generate a tar archive of the build. The output filename will be [build path].tar.gz.',
        'test' => 'FOR MAKE Run a temporary test build and clean up.',
        'translations=languages' => 'FOR MAKE Retrieve translations for the specified comma-separated list of language(s) if available for all projects.',
        'working-copy' => 'FOR MAKE Where possible, retrieve a working copy of projects from their respective repositories.',
      ),
      'examples' => array(
        'drush bakeit A.make B.make' => ' Return merged make file of A.make and B.make',
        'drush bakeit A.make B.bake --type=dev' => ' Return a make file started as A.make and modified by the php in B.bake',
        'cat A.make | drush bakeit B.make --stdin' => ' Return merged make file of A.make and B.make',
        'drush bakeit A.make B.make --output=O.make' => ' place merged make file of A.make and B.make into O.make',
        'drush bakeit A.make B.make --make=PATH' => ' merges make file of A.make and B.make and the use that to build a site at PATH',
        'drush bakeit A.make B.make PATH' => ' merges make file of A.make and B.make and the use that to build a site at PATH',
      ),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
    ),
  );
}

function drush_bake() {
  
  $args = func_get_args();
  $details = pathinfo($args[0]);
  $path = $args[0];
  $name = $details['filename'];
  $build = $a = $args[1]? $a : $name;
  $cmd = "drush bakeit $path $build";
  $cmd .= " --output=$name.make";
  $cmd .= " --working-copy";
  $cmd .= " --prepare-install";
  
  if ($type = drush_get_option('type')) {
    $cmd .= " --type=$type";
  }
  else {
    $cmd .= " --type=dev";
  }
  system($cmd);
  $standard = "$build/profiles/standard";
  $minimal  = "$build/profiles/minimal";
  system("rm -r $standard");
  system("rm -r $minimal");
  $default = "$build/sites/default";
  system("chmod 777 $default/files");
  system("chmod 777 $default/settings.php");


  
}
/*
 * implimentation of hook_drush_help
 */
function bake_drush_help($section) { 
  switch ($section) {
    case 'drush:bake':
      return ("Drush bake");
    break;
    case 'drush:bakeit':
      $help = <<<HELP
Drush bake is a drush command that takes make files and php includes and bake up a new drush make file.
it can have any number of arguments which can be .make files or .bake/.php files.  
If a .make files is passed it is parsed and merged with the previouse file
If a .php file is passed it is include, it has access to the following arrays
  \$info : which is the parsing of the previous make files
  \$config : which is populated with all of options
  the .php file can read and modify these to its hearts content.
  also as this ia a php file it has access to drush_get_option('OPTION') if it wants to use a undocument option.
It will make the site if one provides a dir with --make=DIR or as the last parameter (this must not exists yet)
HELP;
      return dt($help);
  }
}

/*
 * the command for dursh bake
 */
 function drush_bakeit() {
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

  /*
   * This code is to capture our openpublish and openpublic data
   */
  $type = drush_get_option('type');
  if (in_array($args[0], array('openpublish' ,'openpublic'))) {
    $files = array(
      $args[0] .".tpl.bake"
    );
    if(!$type) {
      $type = 'dev';
    }
  }
  //init the info and config arrays
  $make_dir = FALSE;
  $info = array();
  $config = array(
    'type'=> $type,
    'stdin'=> drush_get_option('stdin'),
    'make'=> drush_get_option('make'),
    'output'=> drush_get_option('output'),
  );

  foreach ($files as $file) {
    if (is_file($file)) {
      bake_merge_file($info, $config, $file);
    }
    else {
      $make_dir = $file;
    }
  }
  
  //if we created temp stdinfile remove it
  if ($stdinfile) {
    unlink($stdinfile);
  }

  // BUILD OUTPUT
  $done = FALSE;
  $output = implode("\n",bake_unparse($info));
  if ($out_file = drush_get_option('output')) {
    file_put_contents($out_file, $output);
    $done = TRUE;
  }
  if($make_dir || ($make_dir = drush_get_option('make'))) {
    $tmp_make_file = tempnam("/tmp", "make");
    file_put_contents($tmp_make_file, $output);
    //TODO: I am sure there is a drush way to do this but
    //I do not know what it is
    $make_params = '';
    $make_params .= drush_get_option('version') ? " --version" : "";
    $make_params .= $v = drush_get_option('contrib-destination') ? " --contrib-destination=$v" : "";
    $make_params .= drush_get_option('force-complete') ? " --force-complete" : "";
    $make_params .= drush_get_option('md5') ? " --md5" : "";
    $make_params .= drush_get_option('no-clean') ? " --no-clean" : "";
    $make_params .= drush_get_option('no-core') ? " --no-core" : "";
    $make_params .= drush_get_option('no-patch-txt') ? " --no-patch-txt" : "";
    $make_params .= drush_get_option('prepare-install') ? " --prepare-install" : "";
    $make_params .= drush_get_option('tar') ? " --tar" : "";
    $make_params .= drush_get_option('test') ? " --test" : "";
    $make_params .= $v=drush_get_option('translations') ? " --translations=$v" : "";
    $make_params .= drush_get_option('working-copy') ? " --working-copy" : "";
    system("drush make $tmp_make_file $make_dir $make_params");
    unlink($tmp_make_file);
    $done = TRUE;
  }
  if (!$done) {
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

function bake_merge_file(&$info, &$config, $file) {
  $tmpfile = FALSE;
  if (is_array($file)) {
    $tempdir = tempnam('/tmp','downloaddir');
    unlink($tempdir);
    mkdir($tempdir);
    drush_make_download_factory($file['type'], $file['name'], $file, $tempdir);
    $file = $tempdir. '/'. $file['name'];
    $tmpfile = $file;
  }
  $details = pathinfo($file);
  switch ($details['extension']) {
    case 'php':
    case 'bake':
      //if a php file then let it modify info and config
      bake_include($info, $config, $file);
    break;
    case 'make':
      //if a make file merge it to the current info array
      //TODO: check to insure we have a parseable file
      $info = array_merge_recursive($info, drush_make_parse_info_file($file));
    break;
  }
  if ($tmpfile) {
    unlink($tmpfile);
  }
}
