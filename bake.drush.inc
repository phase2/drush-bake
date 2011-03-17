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
  $args[] = dirname(__FILE__). "/distro.bake";
  call_user_func_array('drush_bakeit', $args);
  
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
 * the command for drush bake
 */
 function drush_bakeit () {
   global $make_files;
  $files = array();
  $stdinfile = FALSE;
  // if we are getting stdin put in a temp file
  // and then add that file to the files array
  if (drush_get_option('stdin')) {
    $stdin = file_get_contents("php://stdin");
      $stdinfile = tempnam("/tmp", "stdin");
   // if (preg_match("/<\?php/", $stdin )) {
    print "Joe $stdin Joe";
    if (TRUE) {
      $stdinfile .= ".bake";
    }
    else {
      $stdinfile .= ".make";
    }
    file_put_contents($stdinfile, $stdin);
    $files[] = $stdinfile;
  }
  
  //grab args and add them to files
  $args = func_get_args();
  $files = array_merge($files, $args);
  //init the info and config arrays
  $make_dir = FALSE;
  $info = array();
  // grab all vars and put them into the config
  $config_keys = array(
    'type',
    'output',
    'stdin' ,
    'make',
    'version',
    'contrib-destination=path',
    'force-complete',
    'md5',
    'no-clean',
    'no-core',
    'no-patch-txt',
    'prepare-install',
    'tar',
    'test',
    'translations=languages',
    'working-copy',
  );
  foreach ($config_keys as $key) {
    $config[$key] = drush_get_option($key);
  }
  if ($extra_config_string = drush_get_option("config")) {
    $extra_config = explode(",", $extra_config_string);
    foreach ($extra_config as $item) {
      list($key, $value) = explode(":", $item);
      $config[$key] = $value;
    }
  }
  $config['files'] = $files;

  foreach ($files as $file) {
    if (is_file($file)) {
      bake_merge_file($info, $config, $file);
    }
    else {
      $config['make'] = $file;
    }
  }
  
  //if we created temp stdinfile remove it
  if ($stdinfile) {
    unlink($stdinfile);
  }

  // BUILD OUTPUT
  $done = FALSE;
  $output = implode("\n",bake_unparse($info));
  if ($out_file = $config['output']) {
    file_put_contents($out_file, $output);
    $done = TRUE;
  }
  if($make_dir = $config['make']) {
    $tmp_make_file = tempnam("/tmp", "make");
    file_put_contents($tmp_make_file, $output);
    $make_options = array();
    $make_options[] = $tmp_make_file;
    $make_options[] = $make_dir;
    $flag_options = array(
      'version', 'force-complete', 'md5', 'no-clean', 'no-core', 'no-patch-txt',
      'prepare-install', 'tar', 'test', 'working-copy',
    );
    $value_options = array(
      'contrib-destination', 'translations',
    );
    foreach($flag_options as $key) {
      if (isset($config[$key]) && $config[$key]) {
        $make_options[] = '--' . $key;
      }
    }
    foreach($value_options as $key) {
      if (isset($config[$key]) && $config[$key]) {
        $make_options[] = $key . '=' . $config[$key];
      }
    }
    drush_backend_invoke('make', $make_options);
    unlink($tmp_make_file);
    foreach ($make_files as $file => $tmpdir) {
      bake_post_make($make_dir, $info, $config, $file);
    }
    $done = TRUE;
  }
  if (!$done) {
    print $output;
  }
  // ok blow away and temp dir with makefiles if they are still around
  foreach ($make_files as $file => $tmpdir) {
    if ($tmpdir) {
      unlink($tmpfile);
    }
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
function bake_make_alter(&$info, &$config, $file) {
  include_once($file);
  $details = pathinfo($file);
  $hook = $details['filename'] ."_make_alter";
  if (function_exists($hook)) {
    $hook($info, $config);
  }
}

function bake_post_make($path, $info, &$post_config, $file) {
  
  include_once($file);
  $details = pathinfo($file);
  $hook = $details['filename'] ."_post_make";
  if (function_exists($hook)) {
    $hook($path, $info, $post_config);
  }
  
}

function bake_merge_file(&$info, &$config, $file) {
  global $make_files;
  $tmpdir = FALSE;
  if (is_array($file)) {
    $tempdir = tempnam('/tmp','downloaddir');
    unlink($tempdir);
    mkdir($tempdir);
    drush_make_download_factory($file['type'], $file['name'], $file, $tempdir);
    $file = $tempdir. '/'. $file['name'];
  }
  $details = pathinfo($file);
  switch ($details['extension']) {
    case 'php':
    case 'bake':
      $details2 = pathinfo($details['filename']);
      if ($details2['extension'] != 'post') {
        //if a php file then let it modify info and config
        bake_make_alter($info, $config, $file);
      }
      // we do not want to blow away bake files becuase they might have post processing
      // so we store all the make files here
      $make_files[$file] = $tmpdir;
    break;
    case 'make':
      //if a make file merge it to the current info array
      //TODO: check to insure we have a parseable file
      $info = array_merge_recursive($info, drush_make_parse_info_file($file));
      // we do not reuse make files so we can blow this away
      if ($tempdir) {
        system("rm $tempdir -r --force");
      }
    break;
  }
}
