#!/usr/bin/perl

# EIOT 定制编译工具

BEGIN { push @INC, './mytools' }

# 需要 File-Path 模块
use File::Find;
use File::Path;                     # mkpath 函数
use File::Copy;                     # Copy 函数
use Digest::MD5 qw(md5 md5_hex);    # 计算文件 MD5
use File::Spec::Functions;
use Cwd;                            # getcwd()
use ParryUtil;    # 提供实用函数, 如: 输出结果文件, 输出 Log

################
my $g_util = ParryUtil->new();    # 提供实用函数
my $curdir = getcwd();            # 当前目录
my $debug  = 1;                   # 调试模式

ClearScreen();                    # 清屏

if ($debug) {
    my $FILE_LOG = $g_util->open_text_file( '>', '~makeEiot.log' );
    $g_util->set_log_handle($FILE_LOG) if ($debug); # 设置输出的 log 文件
}

mylogln("################################");
mylogln( "基础目录：" . getcwd() . "\n" );

################
setupEnv();
################
$myCmd = "mk";

my $configBaseDir = "device/malatamobile";
@chkDirs = qw(mytools ../projects device/malatamobile);
foreach $m (@chkDirs) {
    die "必须的文件夹 \"$m\" 不存在!\n" if ( !-d $m );
}

@arguments = ();
my $product = "";
$action  = "";
$variant = "";
@mOpts   = ();
$ini     = "makeEiot.ini";
$enINI   = ( -e $ini ) ? 1 : 0;
(
    ( $#ARGV < 0 )
      || (
        ( $#ARGV < 1 )
        && (   ( $enINI == 0 )
            && ( lc( $ARGV[0] ) !~ /^[listp|banyan|banyan_x86]/ ) )
      )
) && &Usage;

@actions = qw(
  new n remake r clean c listproject listp
);

@targetBuildVariants = qw(
  user userdebug eng
);

@modules = qw(android kernel dr k preloader pl lk tz);
@orgARGV = @ARGV;

while ( $#ARGV != -1 ) {
    if ( $ARGV[0] =~ /^-h|help/ ) {
        &Usage;
    }
    elsif ( $ARGV[0] =~ /^-(t|te|tee)/ ) {
        $tee = "TRUE";
    }
    elsif ( $ARGV[0] =~ /^-(v|var|variant)=(.*)$/ ) {
        $variant = $2;
    }
    elsif ( $ARGV[0] =~ /^-(o|op|opt)=(.*)$/ ) {
        @mOpts = split( ",", $2 );
    }
    elsif ( $ARGV[0] =~ /^(listp|listproject)/ ) {
        listProject();
        exit 0;
    }
    elsif ( $ARGV[0] =~ /check-env|chk-env/ ) {

        # &chkMustEnv;
        # exit 0;
    }
    else {
        $project = lc( $ARGV[0] );
        my $mkFile = "$configBaseDir/${project}/ProjectConfig.mk";
        mylogln( "Makefile: " . $mkFile );
        if ( !-e $mkFile ) {
            if ( -e $ini ) {
                open( FILE_HANDLE, "<$ini" ) or die "无法打开文件 $ini\n";
                while (<FILE_HANDLE>) {
                    if (/^(\S+)\s*=\s*(\S+)/) {
                        $keyname = $1;
                        $${keyname} = $2;
                    }
                }
                close FILE_HANDLE;
                $project = lc($project);
                $mkFile  = "$configBaseDir/${project}/ProjectConfig.mk";
                mylogln( "ini Makefile: " . $mkFile );
                if ( !-e $mkFile ) {
                    listProject();
                    die "不存在 $ini 指定的 Makefile: $mkFile\n";
                }
            }
        }
        else {
            shift(@ARGV);
        }

        if ( !-e $mkFile ) {
            listProject();
            die "不存在命令行指定的 Makefile: $mkFile\n";
        }

        $action = lc( $ARGV[0] );
        shift(@ARGV);
        @arguments = @ARGV;
        @ARGV      = ();
    }
    shift(@ARGV);

}

mylogln( "action: " . $action );
mylogln( "variant: " . $variant );

# 检查生成类型
if ($variant eq "" ) {
    if ( -e $ini ) {
        open( FILE_HANDLE, "<$ini" ) or die "无法打开文件 $ini\n";
        while (<FILE_HANDLE>) {
            if (/^(\S+)\s*=\s*(\S+)/) {
                $keyname = $1;
                $${keyname} = $2;
            }
        }
        close FILE_HANDLE;
        $variant = lc($build_mode);
        mylogln( "ini variant: " . $variant );
        if ($variant eq "" ) {
            die "不存在 $ini 指定的生成类型: $variant\n使用 $myCmd -h 查看使用说明！\n";
        }
    }
}

$isFound = 0;
foreach $var (@targetBuildVariants) {
    if ( $variant eq $var ) {
        $isFound = 1;
        last;
    }
}
die "不支持的生成类型: $variant.\n使用 $myCmd -h 查看使用说明！\n" if ( $isFound == 0 );

# 检查动作
@acts = split( ",", $action );
foreach $uAct (@acts) {
    $isFound = 0;
    foreach $sAct (@actions) {
        if ( $uAct eq $sAct ) {
            $isFound = 1;
            last;
        }
    }
    die "不支持的动作: $uAct.\n使用 $myCmd -h 查看使用说明！\n" if ( $isFound == 0 );
    
    if ($uAct eq "mm")
    {
        $MM_PATH = shift(@arguments);
        die "指定路径不存在！$MM_PATH" if (! -d $MM_PATH);
        $snod = shift(@arguments) if ($#arguments == 0);
        die "mm 后面的参数只可以是 snod，不能为： $snod" if (($snod ne "") && ($snod ne "snod"));
    }
    
    ($uAct = "new") if ($uAct eq "n");
    ($uAct = "clean") if ($uAct eq "c");
    ($uAct = "remake") if ($uAct eq "r");
}

writeINI();


setEnv("TARGET_BUILD_APPS", "");
setEnv("TARGET_PRODUCT", $product);
setEnv("TARGET_BUILD_VARIANT", $variant);
setEnv("TARGET_BUILD_TYPE", "release");






exit 1;

sub on_found {
    $files{$File::Find::name} = $File::Find::dir;
}

sub get_file_md5 {
    my ($p_file) = @_;    # 获得传入参数
    my $hash = "";

    # 检查源文件是否存在
    my $l_error = !-e $p_file;
    if ($l_error) {
        mylogln("get_file_md5, $p_file 来源文件不存在!");
    }
    else {
        open( FILE, $p_file ) || die "Can't open '$p_file':$!";
        binmode(FILE);
        my $data = <FILE>;
        $hash = md5_hex($data);
        close(FILE);
    }
    mylogln( "MD5:" . $hash . '|' . $p_file );

    return $hash;
}

# 复制文件，目录不存在则创建
sub copy_file {
    my ( $p_from, $p_to ) = @_;    # 获得传入参数

    # 检查源文件是否存在
    my $l_error = !-e $p_from;
    if ($l_error) {
        mylogln("$p_from 来源文件不存在!");
    }
    else {
        # 创建目标目录
        my $l_to_file = just_fname_path($p_to) . '/';    # 截取路径
        if ( !-d $l_to_file ) {
            $l_error = !mkpath($l_to_file);
            if ($l_error) {
                mylogln("创建目标目录失败 $l_to_file: $!");
            }
        }

        # 复制文件
        if ( -d $l_to_file ) {
            $l_error = !copy( $p_from, $l_to_file );
            if ( !$l_error ) {
                mylogln("$p_from 已复制");
            }
            else {
                mylogln("$p_from 复制文件错误! $!");
            }
        }
    }
    return $l_error;
}

sub mylogln {
    my ($msg) = @_;

    $g_util->log("$msg\n") if ($debug);
}

sub mylog {
    my ($msg) = @_;

    $g_util->log("$msg") if ($debug);
}

# 同时在显示器和文件输出
sub output_error {
    my ($p_string) = @_;    # 获得传入参数
    output_message($p_string);

    # output_report($p_string);
    mylogln($p_string);
}

# 在显示器输出消息
sub output_message {
    my ($p_string) = @_;    # 获得传入参数
    $g_util->output_message("$p_string\n");
}

# 在文件输出消息
sub output_report {
    my ($p_string) = @_;    # 获得传入参数
    $g_util->output_result("$p_string\n");
}

sub writeINI {
    @iniFields = qw(project build_mode);
    open( FILE_HANDLE, ">$ini" ) or die "无法打开文件 $ini\n";
    foreach $m (@iniFields) {
        # foreach $i (@mOpts) {
            # @temp = split( "=", "$i" );
            # if ( $temp[0] eq "TARGET_BUILD_VARIANT" ) {
                # $build_mode = $temp[1];
            # }
        # }
        $build_mode = $variant;
        
        if ( $m eq "build_mode" && $build_mode eq "" ) {
            print FILE_HANDLE "$m = eng\n";
        }
        else {
            $value = $${m};
            print FILE_HANDLE "$m = $value\n";
        }
    }
    close FILE_HANDLE;
}

sub p_system
{
  my ($cmd) = @_;
  my ($debugp) = 0;
  my $result;
  ($debugp != 0) && print("$cmd\n");
  ($performanceChk == 1) && print &CurrTimeStr . " system $cmd\n";
  $result = system("$cmd");
  ($performanceChk == 1) && print &CurrTimeStr . " exit $cmd\n";
  return $result;
}

sub listProject {
    if ( opendir( DH, $configBaseDir ) ) {
        output_message("可用的项目:");
        output_message("==========================");
        my $i = 1;
        foreach $file ( @currdir_files =
            readdir DH )    # 列出所有目录或文件
        {
            next if ( $file eq "." or $file eq ".." );
            my $newPath = catfile( $configBaseDir, $file );
            next if ( !-d $newPath );    # 如果不是目录
            output_message( "     " . $i . ". " . $file );
            $i += 1;
        }
        output_message("");
    }
}

sub setupEnv{
    my $JAVA_HOME = '/usr/lib/jvm/java-7-openjdk-amd64';
    my $PWD = $curdir;

    setEnv('JAVA_HOME', $JAVA_HOME);
    setEnv('BUILD_ENV_SEQUENCE_NUMBER', '10');
    setEnv('PATH', "$JAVA_HOME/bin:$PWD/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.8/bin:$PWD/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin:$PATH");
}

sub setEnv{
    my($key, $value) = @_;		# 获得传入参数
    $ENV{$key} = $value;
}

sub Usage {
    warn << "__END_OF_USAGE";
使用说明: (makeEiot|mk) [选项] 生成类型 项目 动作 [模块]

选项:
  -t, -tee      : 输出调试信息。
  -o, -opt=参数
                : 传递额外的参数，多个参数以“,”分隔。

  -h, -help     : 显示使用说明。

生成类型:
  -v, -variant=(user|userdebug|eng)
                : 生成类型。
  
项目:
  可用的项目。

动作:
  listp, listproject
                : 列出可用项目。

  check-env     : Check if build environment is ready.
  check-dep     : Check feature dependency.
  n, new        : Clean and perform a full build.
  c, clean      : Clean the immediate files(such as, objects, libraries etc.).
  r, remake     : Rebuild(target will be updated if any dependency updats).
  mrproper      : Remove all generated files + config + various backup files in Kbuild process.
  bm_new        : "new" + GNU make's "-k"(出错后继续编译) feature.
  bm_remake     : "remake" + GNU make's "-k"(出错后继续编译) feature.
  mm            : Build module through Android native command "mm"

  emigen        : Generate EMI setting source code.
  nandgen       : Generate supported NAND flash device list.
  codegen       : Generate trace DB(for META/Cather etc. tools used).
  drvgen        : Generate driver customization source.
  custgen       : Generate customization source.
  javaoptgen    : Generate the global java options.
  ptgen         : Generate partition setting header & scatter file.
  bindergen     : Generate binder related information

  sign-image    : Sign all the image generated.
  encrypt-image : Encrypt all the image generated.
  update-api    : Android default build action
                  (be executed if system setting or anything removed from API).
  check-modem   : Check modem image consistency.
  upadte-modem  : Update modem image located in system.img.
  modem-info    : Show modem version
  gen-relkey    : Generate releasekey for application signing.
  check-appres  : Check unused application resource.

  sdk           : Build sdk package.
  win_sdk       : Build sdk package with a few Windows tools.
  banyan        : Build MTK sdk addon.
  banyan_x86    : Build MTK sdk x86 addon.
  cts           : Build cts package.
  bootimage     : Build boot image(boot.img).
  cacheimage    : Build cache image(cache.img).
  systemimage   : Build system image(system.img).
  snod          : Build system image without dependency.
                  (that is, ONLY pack the system image, NOT checking its dependencies.)
  recoveryimage : Build recovery image(recovery.img).
  secroimage    : Build secro image(secro.img).
  factoryimage  : Build factory image(factory.img).
  userdataimage : Build userdata image(userdata.img).
  userdataimage-nodeps
                : Build userdata image without dependency.
                  (that is, ONLY pack the userdata image, NOT checking its dependencies.)
  dump-products : Dump products related configuration(PRODUCT_PACKAGE,PRODUCT_NAME ect.)
  target-files-package
                : Build the target files package.
                  (A zip of the directories that map to the target filesystem.
                   This zip can be used to create an OTA package or filesystem image
                   as a post-build step.)
  updatepackage : Build the update package.
  dist          : Build distribution package.

Modules:
  pl, preloader : Specify to build preloader.
  lk            : Specify to build little kernel.
  tz, trustzone : Specify to build trusted execution environment.
  k,  kernel    : Specify to build kernel.
  dr, android   : Specify to build android.
  NULL          : Specify to build all components/modules in default.
  k <module path>
                : Specify to build kernel component/module with the source path.
  dr <module name>
                : Specify to build android component/module with module name.

Other tools:
  prebuilts/misc/linux-x86/ccache/ccache -M 10G
                : Set CCACHE pool size to 10GB

Example:
  ./mk -t e1k emigen
                : Generate EMI setting source code.
  ./mk -o=TARGET_BUILD_VARIANT=user e1k n
                : Start a user mode full build.
  ./mk listp    : List all available projects.
  ./mk e1k bootimage
                : Build bootimage for e1k project.
  ./mk e1k bm_new k
                : Build kernel for e1k project.
  ./makeMtk e1k c,bm_remake pl k
                : Clean & Build preloader and kernel for e1k project. 
  ./makeMtk e1k n k kernel/xxx/xxx
                : Build(full build) kernel component/module 
                  under the path "kernel/xxx/xxx" for e1k project.
  ./makeMtk e1k r dr Gallery
                : Rebuild android module named Gallery for e1k project.
  ./makeMtk e1k mm packages/apps/Settings
    : Change Directory to packages/apps/Settings and execute "mm"


__END_OF_USAGE

    exit 1;
}
