#!/usr/bin/perl

# == 定制编译工具 ==
#
# 支持简化命令，new 编译后的代码目录，不需要再输入'项目'和'编译模式'参数
# 支持使用强力压缩格式(7z)打包并发布软件
# 支持定制发布目录，并生成 Windows 格式的路径，释放写软件释放邮件时引用
# 支持项目配置文件，精细化各种项目属性，可灵活定制软件生成名称，释放路径的规则，避免软件名称的随意性
# 支持延时编译
# 支持参数组，释放软件只需要输入一个参数组即可以符合释放软件参数要求
# 支持以参数启用 git 下载代码，出错时不进行编译，避免编译结果并非期望的
# 支持随时打包发布软件（release 动作），构成发布软件名称的时间依据Bin的最新时间生成，即 Bin 文件没有改动，则不会重复打包发布
# 钱家洪, 2018.08.06
#
# 修改记录：
# 钱家洪, 2018.12.10 支持多命令动作，以 "," 连接多个命令；lisp 动作可以传入关键字参数可以搜索项目；支持导出 OTA 文件到发布目录。

BEGIN { push @INC, './mytools' }

use POSIX ":sys_wait_h";            # WNOHANG, 多进程相关
use Data::Dumper;                   # 打印各种变量, 调试用
use Sys::Hostname;                  # Hostname
use Sys::HostIP;                    # 获取本机IP
use Time::Piece;                    # strftime, 时间相关
use File::Find;                     # 枚举指定目录及其子目录的文件
use File::Path;                     # mkpath, rmtree 函数
use File::Copy;                     # Copy 函数
use File::Spec::Functions;          # 文件路径字符操作
use Cwd;                            # getcwd()

use ParryMTKAndroidUtil;	        # 提供 MTK Android 实用函数
use ParryUtil;                      # 提供实用函数, 如: 输出结果文件, 输出 Log
use ParryCopyFolderUtil;            # copyFolder 复制目录

################
my $g_util = ParryUtil->new();      # 提供实用函数
my $rootDir = getcwd();             # 当前目录
my $debug  = 0;                     # 调试模式
my $performanceChk  = 1;            # 性能检测
my $threadNum = 40;                 # 线程数

my $projectsBaseDir = '../projects/';       # 抽取出来的项目文件
my @chkDirs = ('mytools', $projectsBaseDir);
foreach my $m (@chkDirs) {
    die "必须的文件夹 \"$m\" 不存在!\n" if ( !-d $m );
}

my %binListHashMap = (
    'shell' => '/bin/bash',         # Linux Shell 脚本执行程序
    'mk' => '/usr/bin/mk',          # 快捷执行程序, 用于执行目录的 mk.sh
    'linux-zip' => '/usr/bin/zip',  # Linux 内置的 zip
    '7zip' => '/usr/bin/7z'         # 7zip 压缩程序
);

# 需要保存的编译参数
my %buildConfigFieldHashMap = (
    'project' => '项目',
    'buildMode' => '编译模式'
);

# 打包相关的变量
my %packageBinConfigHashMap = (
    'sourceBase' => './out/target/product',   # Android 编译结果(bin)输出的基础目录
    'includeDatabase' => 1      # 打包文件包括数据库
);

# 从项目配置文件读取的信息
%gBuildConfigHashMap = ();
our %gPublicConfigHashMap = (
    'tmpDir' => catfile(getLinuxHomePath(), '.tmp'),
    'basePath' => '/home/share/sw_release',       # 发布目录
    'basePathWin' => '\\\\[host]\share\sw_release',  # 发布目录(Windows)
);

# 可用的编译动作
my @actions = qw(new n remake r clean c package p);
my @actionsNative = qw(mm snod update-api otapackage ota);
push(@actions, @actionsNative);

my @makeTargets = qw(framework services libandroid_runtime files
    systemimage bootimage recoveryimage userdataimage secroimage cacheimage);

my @arguments = ();
my @mOpts   = ();
my $shellFile = '~build.sh';
my $projectCfg = 'config.ini';          # 项目配置文件
my $baseProjectCfg = catfile($projectsBaseDir, 'base_config.ini'); #基本项目配置文件
my $publicConfig = catfile(getLinuxHomePath(),  '.make_public_config.ini');     # 个人公共编译配置
my $buildConfigIni = '~make.ini';       # 编译命令参数配置文件
my $enBuildConfig = ( -e $buildConfigIni ) ? 1 : 0;   # 已启用'编译命令参数配置文件'

my $userConfirmDelaySec = 30;           # 用户确认编译参数的时间（秒）

my $project = "";
my $buildMode = "userdebug";          # 默认编译模式
my $product = "";
my $action  = "";

my $isOta = 0;
my $isPackage = 0;
my $isUpdateApi = 0;
my $isGitPull = 0;
my $delayRawString = '';
my $delayString = '';
my $enDelay = 0;
my $delaySeconds = 0;      # 延时多少秒


##############
# 程序开始
##############
&init();  
&main();

# print $ENV{"x"}."make.pl";
# $ENV{"x"} = 'sdfsdf';

exit 1;
##############
# 程序结束
##############

sub init()
{
    if ($debug) {
        my $FILE_LOG = $g_util->open_text_file( '>', '~makeDebug.log' );
        $g_util->set_log_handle($FILE_LOG); # 设置输出的 log 文件
    }

    enabledCCache();

    # 读取公共配置
    if (!-f $publicConfig){
        my $localIp = getLocalIp();
        $gPublicConfigHashMap{'basePathWin'} =~s/\[host\]/$localIp/;
        saveConfig($publicConfig, %gPublicConfigHashMap);
    } else {
        restoreConfig($publicConfig, \%gPublicConfigHashMap);
    }
}

sub main
{
    # ClearScreen();                    # 清屏

    # 分析命令，获取参数
    while ( $#ARGV != -1 ) {
        if ( $ARGV[0] =~ /^-(h|help)$/ ) {
            Usage();
        } elsif ( $ARGV[0] =~ /^-debug$/ ) {
            $debug = 1;
        } elsif ( $ARGV[0] =~ /^-(p|project)=(.*)$/ ) {
            mylogln("project=".$2);
            $project = $2;
        } elsif ( $ARGV[0] =~ /^-gitpull$/ ) {
            mylogln("gitpull");
            $isGitPull = 1;
        } elsif ( $ARGV[0] =~ /^-(d|delay)=(.*)$/ ) {
            $delayRawString = "-$1=$2";
            $delayString = $2;
            mylogln($delayRawString);
        } elsif ( $ARGV[0] =~ /^-(m|buildMode)=(.*)$/ ) {
            mylogln("buildMode=".$2 );
            $buildMode = $2;
        # } elsif ( $ARGV[0] =~ /^-(o|op|opt)=(.*)$/ ) {
        #     @mOpts = split( ",", $2 );
        } elsif ( $ARGV[0] =~ /^check-env|chk-env$/ ) {
            chkMustEnv();
            exit 0;
        } else {
            $action = lc(shift(@ARGV));
            mylogln("action: " . $action);

            # 收集剩余的参数
            if (@ARGV > 0) {
                @arguments = @ARGV;
                @ARGV      = ();
            }
        }

        shift(@ARGV);
    }

    if (!chkMustEnv(1)){
        output_error("缺少必要的程序，无法继续执行！");
        showMessageLn("mk chk-env 检查编译环境\n");
        exit 1;
    }    

##############

    # 编译动作可以多个，以 ',' 分隔
    my @acts = split(",", $action);
    if (@acts < 1) {
        Usage();
        exit 0;
    }

    if ($action =~ /^(listp|listproject)$/) {
        if (@arguments > 0) {
            listProject(shift(@arguments));
        } else {
            listProject();
        }
        exit 0;
    }

    if ($enBuildConfig) {
        showMessageLn("################################");
        restoreConfig($buildConfigIni, \%gBuildConfigHashMap);
        showMessageLn("已启用配置文件: " . $buildConfigIni);
        showMessageLn("保存的参数: ");
        while (my($key, $value) = each(%gBuildConfigHashMap)) {
            my $keyDesc = $buildConfigFieldHashMap{$key};
            if (defined($keyDesc)) {
                showMessageLn('  ' . $keyDesc . ' = ' . $value);
            } else {
                showMessageLn('  ' . $key . ' = ' . $value);
            }
        }
        showMessageLn("################################");

        if (exists($gBuildConfigHashMap{'project'})) {
            $project = $gBuildConfigHashMap{'project'};
        }
        
        if (exists($gBuildConfigHashMap{'buildMode'})) {
            $buildMode = $gBuildConfigHashMap{'buildMode'};
        }      
    }
    
    mylogln("project: " . $project);

    showMessageLn("基础目录：" . $rootDir);
    showMessageLn("编译模式：" . $buildMode);
    showMessageLn("编译动作：" . $action);
    showMessageLn("###############################");

    if ($project eq "") {
        output_error("需要指定项目名称!\n  示例: mk -p=PrjName");
        showMessageLn("  mk -h 查看使用说明\n");
        listProject();
        exit 0;
    }

    my $projectDir = catfile($projectsBaseDir, $project);
    if (!-d $projectDir) {
        output_error("项目不存在: ".$projectDir);
        listProject($project);
        exit 0;
    }

    my $codeFilesDir = catfile($projectDir, 'codeFiles');
    if (!-d $codeFilesDir) {
        output_error("项目代码目录不存在: ".$codeFilesDir);
        exit 0;
    }

    my $prjConfigFile = catfile($projectDir, $projectCfg);
    if (!-f $prjConfigFile) {
        output_error("项目配置文件不存在: ".$prjConfigFile);
        exit 0;
    }

    showMessageLn("项目目录    ：" . $projectDir);
    showMessageLn("基本配置文件：" . $baseProjectCfg);
    showMessageLn("项目配置文件：" . $prjConfigFile);
    showMessageLn("项目代码目录：" . $codeFilesDir);
    showMessageLn("################################");

    if ($buildMode =~ /^(u|d|e|user|userdebug|eng)$/) {
        if ($1 eq 'u') {
           $buildMode = 'user'; 
        }
        if ($1 eq 'd') {
           $buildMode = 'userdebug'; 
        }
        if ($1 eq 'e') {
           $buildMode = 'eng'; 
        }
    } else {
        output_error("编译模式必须为：u, d, e, user, userdebug, eng");
        showMessageLn("  mk -h 查看使用说明\n");
        exit 0;
    }
    
    # 检查延时文本
    my $newDelayString = '';
    if ($delayRawString){
        if ($delayString =~ /([0-9]+)([m|h])$/){
            $delaySeconds = $1 * 60;
            
            my $delayUnit = 'm';  # 延时单位
            if ($2 eq 'h'){
                $delaySeconds *= 60;
                $delayUnit = 'h';
            }
            $newDelayString = "$1 ". ($delayUnit eq 'm' ? '分钟': '小时');
            mylogln("延时：$delaySeconds 秒");
        } else {
            output_error("$delayRawString, 延时多少分钟/小时编译，格式：-d=n(m|h)， -delay=n(m|h) ，n 为整数");
            showMessageLn("示例：-d=30m 为延时 30 分钟，-d=2h 为延时 2 小时");
            showMessageLn("  mk -h 查看使用说明\n");
            exit 0;
        }
    }

    showMessageLn("延时编译    ：$newDelayString") if ($delaySeconds > 0);
    showMessageLn("################################") if ($delaySeconds > 0);

    ############################################

    $gBuildConfigHashMap{'project'} = $project;
    $gBuildConfigHashMap{'buildMode'} = $buildMode;
    saveConfig($buildConfigIni, %gBuildConfigHashMap);

    # 复制定制的文件
    if (copyFolder($codeFilesDir, $rootDir) < 0) {
        output_error("复制项目文件失败!");
    } else {
        # 读取项目配置
        my @orderBase = restoreConfig($baseProjectCfg, \%prjConfigHaseMap);
        my @orderPrj = restoreConfig($prjConfigFile, \%prjConfigHaseMap, 1);
        my @orderAll = (@orderBase, @orderPrj);
        mylogln(Data::Dumper->Dump([\@orderAll], ['*' ."orderAll"]));

        # 找出包括宏的字段, 并保持原文件的先后顺序
        my @macroList = ();  # 包含宏的字段值
        my @tempHashMap = ();
        foreach my $key (@orderAll) {
            # 如果 @macroList 中已存在就不添加
            if (! $tempHashMap{$key}) {
                my $value = $prjConfigHaseMap{$key};
                if ($value and ($value =~ /\{(.+)\}/)) {
                    push(@macroList, $key);
                    $tempHashMap{$key} = '';
                }
            }
        }
        mylogln(Data::Dumper->Dump([\@macroList], ['*' ."macroList"]));

        # 执行宏替换
        mylogln('执行宏替换');
        foreach my $key (@macroList) {
                mylogln("### KEY: $key\n----------------------");
                my $value = $prjConfigHaseMap{$key};
                my $newValue = replaceConfigField($value, %prjConfigHaseMap);
                $prjConfigHaseMap{$key} = $newValue;
        }
        mylogln(Data::Dumper->Dump([\%prjConfigHaseMap], ['*' ."prjConfigHaseMap"]));

        # 软件释放名称
        my $configKey = 'release';
        my $releaseName = $prjConfigHaseMap{$configKey};
        if (defined($releaseName)){
            showMessageLn('释放软件名称：' . $releaseName);
        } else {
            $releaseName = '';
            output_error("必须定义'$configKey'字段, 配置文件: $prjConfigFile");
            exit 0;
        }

        # 软件释放路径
        $configKey = 'release_path';
        my $configReleasePath = $prjConfigHaseMap{$configKey};
        my $releasePath = $gPublicConfigHashMap{'basePath'};        # 发布软件的目录
        if (defined($configReleasePath)){
             $releasePath = catfile($releasePath, $configReleasePath);
            showMessageLn('释放目录    ：' . $releasePath . " (可以在'公共配置'文件修改基础目录)");
            showMessageLn("公共配置    ：" . $publicConfig);            
        } else {
            output_error("必须定义'$configKey'字段, 配置文件: $prjConfigFile");
            exit 0;  
        }
        showMessageLn("################################");
        showMessageLn('');

        ### 即将开始长时间的任务 ###
        # 非延时编译时，让用户确认以上的命令参数正确
        if ($delaySeconds == 0) {
            showMessageLn("确认以上信息无误, 并开始执行? <回车> 确认，<Ctrl+C> 取消执行");
            system("sleep 0.01");     # 让此前的信息全部输出到命令行窗口
            waitInput($userConfirmDelaySec);
        }

        my $error = 0;
        
        # 从 git 服务器下载文件
        if ($isGitPull){
            $error = runSystemCmd('git pull');
            if ($error){
                output_error('下载 Git 代码失败!');
            }
        }
        
        my $makeCmd = '';
        if (!$error) {
            # 检查编译动作是否合法，转换简写命令为长命令          
            $makeCmd = "source build/envsetup.sh\n";
            $makeCmd .= "lunch " . $prjConfigHaseMap{'mtk_product'} . "-$buildMode\n";
            my $baseCmd = "make -j" . $threadNum . ' ';

            foreach $uAct (@acts) {
                my $isFound = 0;
                foreach $sAct (@actions) {
                    if ($uAct eq $sAct) {
                        $isFound = 1;
                        last;
                    }
                }

                if (! $isFound) {
                    output_error("编译动作不能为 $uAct，可选动作：" . join(', ', @actions));
                    if ( $enBuildConfig ) {
                        showMessageLn("注意：new 编译后，会保存编译的参数，不必再输入'项目'和'编译模式'参数，如需变更，请先删除文件：$buildConfigIni");
                        showMessageLn("　　  这种情况的命令格式为： mk [选项] 编译动作 [模块|路径]\n");
                    }
                    showMessageLn("  mk -h 查看使用说明\n");
                    exit 0;
                }

                my $mmPath = '';
                my $snod = '';
                if ($uAct eq "mm") {
                    $mmPath = shift(@arguments);
                    if (!$mmPath) {
                        output_error("mm 必须指定路径! ");
                        exit 0;
                    }
                    if (! -d $mmPath) {
                        output_error("mm 所需要的路径不存在! " . $mmPath);
                        exit 0;
                    }

                    if (@arguments == 1) {
                        $snod = shift(@arguments);
                    }

                    if (($snod ne '') && ($snod ne "snod")) {
                        output_error("mm 的参数后面必须跟 snod, 并非 $snod");
                        exit 0;
                    }
                }

                ($uAct = "new") if ($uAct eq "n");
                ($uAct = "remake") if ($uAct eq "r");
                ($uAct = "clean") if ($uAct eq "c");
                ($uAct = "package") if ($uAct eq "p");
                ($uAct = "otapackage") if ($uAct eq "ota");
                
                if ($uAct =~ /^(package)$/) {
                    $isPackage = 1 ;
                    next;
                }

                $isOta = 1 if ($uAct =~ /^(otapackage)$/);

                if ($uAct =~ /^(mm)$/) {
                    $makeCmd .= 'cd ' . $mmPath . "\n";
                    $makeCmd .= "mm\n";
                    $makeCmd .= 'cd ' . $rootDir . "\n";
                    $makeCmd .= $baseCmd . $snod;
                } else {
                    if ($uAct =~ /^(new|clean|remake)$/) {
                        $makeCmd .= $baseCmd;

                        if ($uAct =~ /^(clean|new)$/) {
                            $makeCmd .= "clean ";
                        }
                        
                        if ($uAct =~ /^(new)$/) {
                            $makeCmd .= "\n" . $baseCmd;
                        } else {
                            if (@arguments > 0) {
                                $makeCmd .= join(' ', @arguments);
                            }
                        }

                        $makeCmd .= "\n";
                    } else { 
                        $makeCmd .= $baseCmd . $uAct . "\n";
                    }
                }
            }
        }

        if (!$error and buildShellFile($shellFile, $makeCmd)) {
            # 延时执行
            if ($delaySeconds > 0) {
                my $currTime = Time::Piece->new;
                my $nextTime   = $currTime + ($delaySeconds);
                showMessageLn("已就绪！将于 " . $nextTime->strftime("%Y.%m.%d %H:%M") . ' 执行编译 (现在时间: ' . $currTime->strftime("%Y.%m.%d %H:%M") .')');
                showMessageLn('<Ctrl+C>取消执行');

                # 不能使用 perl sleep 函数，结束前，此前输出到命令行窗口的内容不显示
                system("sleep ".$delaySeconds);  
            }

            my $binPath = catfile($packageBinConfigHashMap{'sourceBase'}, $prjConfigHaseMap{'mtk_model'});
            $error = runSystemCmd($binListHashMap{'shell'} . ' ' . $shellFile);
            if (!$error and $isPackage){
                my $archivePath = package_bin($releaseName, $binPath, $releasePath, $buildMode, $packageBinConfigHashMap{'includeDatabase'}, $prjConfigHaseMap{'mtk_product'}, $isOta);
                if ($archivePath ne ""){
                    $archivePath =~ s/$gPublicConfigHashMap{'basePath'}/$gPublicConfigHashMap{'basePathWin'}/;
                    $archivePath =~ s#/#\\#g;
                    showMessageLn('释放路径：' . $archivePath);
                }
            }

            # 清除所有 verified 的 bin，下载工具会自动选择这些 bin 下载，导致软件异常
            system("rm -f $binPath/*verified*");
        }

        showMessageLn("================\n主进程结束了") if ($debug);
        showCurrentTime();
    }
}

sub showCountdown
{
    my($seconds, $message) = @_;
    # showMessageLn("==showCountdown==");

    # system("tput civis");   # 隐藏光标
    # system("tput cnorm");   # 显示光标

    my $i = $seconds;
    showMessageLn("");
    showMessage(sprintf("%5d %s", $i, $message));
    eval {
        $SIG {ALRM} = sub {
            $i--;
            system('printf "\r\a"');
            showMessage(sprintf("%5d %s", $i, $message));
            
            if ($i == 0){
                sleep(1);
                die "\n";
            } else {
                alarm(1);   #超时处理
            }
        };

        alarm(1); #1秒后进入超时处理
        while(1) {}  
    };  
    showMessageLn("\n");
}

# 等待用户按键输入
sub waitInput
{
    my($seconds) = @_;

    my $countdownPid = 0;

    if ($seconds > 0){
        my $pid = fork();
        if (!defined($pid)) {
            output_error("Error in fork: $!");
            exit 1;
        } 
    
        if ($pid == 0) {
            # showMessageLn("这是子进程 My pid = $$");
            eval {
                local $SIG{'KILL'} = sub {
                    # showMessageLn("==> $$ $countdownPid KILL");
                    die "KILL interrupted, exiting...";
                };            
    
                showCountdown($seconds, "秒后继续执行");
            };
            # print "==fork showCountdown end==\n";
            exit 0;
        } else {
            # showMessageLn("这是主进程 My pid = $$, and my child's pid = $pid");
            $countdownPid = $pid;
        }    
    }


    my $result = eval{
        # showMessageLn("这是子进程eval My pid = $$");
        local $SIG{ALRM} = sub {
            sleep(1); 
            # showMessageLn("SIG, 终止 eval 块"); 
            # sleep(1); 
            # 终止 eval 块
            die "timeout";
        };
        
        # 设置超时时间
        alarm($seconds + 1) if ($seconds > 0);

        # 等待用户输入
        chomp(my $strInput = <STDIN>);
        if ($countdownPid > 0) {
            kill('KILL', $countdownPid);
            showMessageLn("");
            alarm(0);
            sleep(0.1);
        }
        return $strInput;
    };

    if ($debug) {
        if($@ and $@ =~ /timeout/i) {
            showMessageLn("已超时\n");
        # } else {
        #     showMessageLn("非超时\n");
        }
    }
    
    while ((waitpid(-1, WNOHANG)) > 0) {}
    return $result;
}

sub listProject
{
    my($keywork) = @_;		# 获得传入参数
    my @projectNames = ();
    my @filterProjectNames = ();

	if (opendir (DH, $projectsBaseDir))	{
		foreach $l_file(readdir DH)	{	# 列出所有目录或文件	
			next if ($l_file eq "." or $l_file eq "..");
            
            if (-d catfile($projectsBaseDir, $l_file)) {
                push(@projectNames, $l_file);
                if ($keywork && ($l_file =~ /$keywork/i)) {
                    push(@filterProjectNames, $l_file);
                }
			}
		}
	}
	close(DH);						# 关闭目录

    my @displayNames = ();
    if (@filterProjectNames > 0) {
        showMessageLn("相似的项目:");    
        @displayNames = sort(@filterProjectNames);
    } else {
        showMessageLn("可用的项目:");
        @displayNames = sort(@projectNames);
    }

    foreach $name (@displayNames) {
        showMessageLn("  " . $name);
    }

	showMessageLn("");
}

# 生成编译命令集
sub buildShellFile {
    my($file, $shellCmdLine) = @_;		# 获得传入参数
 
    open (FILE_HANDLE, ">$file") or die "无法创建文件: $file\n";
    print FILE_HANDLE $shellCmdLine;
    close(FILE_HANDLE);
    
    return ( -e $file ) ? 1 : 0;
}

# 读取配置文件通用函数
#   fileIni, 配置文件
#   configHashMap, 要存储配置参数的 hashMap 指针
#   appendMode, 不清除 configHashMap 的已有数据
sub restoreConfig
{
    my ($fileIni, $configHashMap, $appendMode) = @_;

    my @arrKeys = ();

    # 追加模式, 不清除
    if (defined($appendMode) and !$appendMode) {
        %{$configHashMap} = ();
    }

    mylogln('restoreConfig:' . $fileIni);
    if (-f $fileIni) {
        open( FILE_HANDLE, "<$fileIni" ) or die "无法打开 $fileIni\n";
        while (<FILE_HANDLE>) {
            chomp();
            next if (/^\s*(;|#).*/);
            if (/^\s*(\S+)\s*=\s*(\S+)\s*/) {
                $configHashMap->{$1} = $2;
                push(@arrKeys, $1);
                mylogln("  $1 = $2");
            }
        }
        mylogln("");
        close FILE_HANDLE;
    } else {
        output_error('文件不存在 ' . $fileIni) and die;
    }

    return @arrKeys;
}

# 保存参数到文件
sub saveConfig
{
    my ($fileIni, %configHashMap) = @_;

    open (FILE_HANDLE, ">$fileIni") or die "无法写文件 $fileIni\n";

    mylogln('saveConfig:' . $fileIni);
    while (my($key, $value) = each(%configHashMap)) {
        print FILE_HANDLE "$key = $value\n";
    }

    close FILE_HANDLE;
}

# 替换配置参数的"宏"(以{}包括的字符串)
sub replaceConfigField
{
    my ($fieldValue, %configHashMap) = @_;

    # 找出需要替换的字符串
    my $i = 0;
    my %tempHashMap = ();  # 避免重复
    my $newValue = $fieldValue;
    mylogln("原值： $newValue");
    while ($newValue =~ /\{(.+?)\}/) {
        my $tmpKey = $1;
        $tempHashMap{$tmpKey} = '';
        $newValue =~ s/\{$tmpKey\}//;

        # 避免死循环
        last if ($i++ > 100);
    }

    # 查询可替换的内容，并将其替换
    $newValue = $fieldValue;
    foreach my $key (keys %tempHashMap) {
        # mylogln("key=$key");
        my $value = $prjConfigHaseMap{$key};
        if ($value) {
            $newValue =~ s/\{$key\}/$value/;
            # mylogln("{$key}=>$value, newValue=$newValue");
        } else {
            mylogln("*** {$key}=>找不到");
        }
    }
    mylogln("替换后：$newValue");
    mylogln('');
    
    return $newValue;
}

# 启用 CCache
sub enabledCCache
{
    my $hostServer = hostname();
    showMessageLn('服务器'.$hostServer) if ($debug);

    if ((exists $ENV{"USE_CCACHE"}) && ($ENV{"USE_CCACHE"} eq "")) {
        showMessageLn("CCache: 已被设置为禁用");
    } elsif (-d $ENV{"HOME"} . "/.ccache") {
        # 默认启用 CCache
        $ENV{"USE_CCACHE"} = 1;
        showMessageLn("CCache: 已启用") if ($debug);
    } else {
        showMessageLn("警告: CCache 未初始化而被禁用");
        showMessageLn("请先执行命令设置 CCache 池大小： prebuilts/misc/linux-x86/ccache/ccache -M 10G");
    }
    if ((exists $ENV{"USE_DXCACHE"}) && ($ENV{"USE_DXCACHE"} eq "")) {
    }
    elsif (-d $ENV{"HOME"} . "/.dxcache") {
        $ENV{"USE_DXCACHE"} = 1;
    }
}

sub setTempDir
{
    my($tmpDirPath) = @_;
    if ((!-d $tmpDirPath) and (!mkpath($tmpDirPath))) {
        output_error("创建临时目录失败 $tmpDirPath: $!");
    }
    system("chmod 777 '$tmpDirPath'");
    setSystemEnv('TMPDIR', $tmpDirPath);  # 设置临时目录
}

# 运行操作系统命令，成功执行返回 0
sub runSystemCmd
{
    my ($cmd) = @_;
    my $result;
    
    showMessageLn("执行命令：$cmd") if ($debug != 0);
    my $startTime = localtime() if ($performanceChk);
    # $result = system("$cmd") >> 8;
    $result = system("$cmd");
    # $result = `$cmd`;     # 不会输出结果和进度到屏幕
    # showMessageLn('执行结果: ' . $result) if ($debug != 0);
    if ($performanceChk) {
        my $seconds = (localtime() - $startTime);
        my $hour = int($seconds / 60 / 60);
        my $minute = int($seconds / 60 % 60);
        my $second = $seconds % 60;
        showMessageLn('耗时：' . sprintf("%02d:%02d:%02d", $hour, $minute, $second));
    }

    return $result;
}

sub getLocalIp
{
    # my $cmd = '/sbin/ifconfig -a|grep inet|grep -v 127.0.0.1|grep -v inet6|awk \'{print $2}\'|tr -d "addr:"'
    # my $result = `$cmd`; 
    
    my $result = '';
    my $hostIp = Sys::HostIP->new;
    my $allIps = $hostIp->ips;
    foreach my $ip( @{$allIps}){
        next if ($ip eq '127.0.0.1');
        $result = $ip;
        last;
    }

    return $result;
}

sub getLinuxHomePath
{
    return $ENV{"HOME"};
}

sub hashDumper
{
    my ($hashMapName, %hashMap) = @_;

    $Data::Dumper::Maxdepth = 10; # default is 0
    $content = Data::Dumper->Dump([\%hashMap], ['*' ."$hashMapName"]);

    return $content;
}

sub mylogln
{
    my ($msg) = @_;
    
    $g_util->log("$msg\n") if ($debug);
}

sub mylog
{
    my ($msg) = @_;
    
    $g_util->log("$msg") if ($debug);
}

# 同时在显示器和文件输出
sub output_error
{
	my($p_string) = @_;		# 获得传入参数
	showMessageLn("\n*** ERROR: " . $p_string . "\n");
	# output_report($p_string);
    mylogln("\n*** ERROR: " . $p_string . "\n");
}

# 在文件输出消息
sub output_report
{
	my($p_string) = @_;		# 获得传入参数
	$g_util->output_result("$p_string\n");
}

sub showCurrentTime
{
    my $currTime = localtime();
    showMessageLn(sprintf("[%s]", $currTime->strftime("%Y.%m.%d %H:%M")));
}
####################################################################
# 获取文件主杆，含扩展名
sub just_fname_stem
{
	my($p_fullpath) = @_;		# 获得传入参数
	$p_fullpath =~ s#\\#/#g;	# 把 \ 替换为 /, 以兼容两类路径分隔符号
	return substr($p_fullpath, rindex($p_fullpath, "/")+1);  #  截取文件名，含扩展名
}
	
# 获取文件扩展名，不含点号
sub just_fname_ext
{
	my($p_fullpath) = @_;		# 获得传入参数
	my $fname = "";
	my $pos = -1;

	$p_fullpath =~ s#\\#/#g;	# 把 \ 替换为 /, 以兼容两类路径分隔符号
	$fname = substr($p_fullpath, rindex($p_fullpath, "/")+1);  #  截取文件名，含扩展名
	
	$pos = rindex($fname, "\.");
	return ($pos == -1 ? "" : substr($fname, $pos + 1));  #  截取扩展名
}

# 获取文件路径
sub just_fname_path
{
	my($p_fullpath) = @_;		# 获得传入参数
	$p_fullpath =~ s#\\#/#g;	# 把 \ 替换为 /, 以兼容两类路径分隔符号
	return substr($p_fullpath, 0, rindex($p_fullpath, "/"));  #  截取路径
}

# 设置系统变量
sub setSystemEnv{
    my($key, $value) = @_;		# 获得传入参数
    $ENV{$key} = $value;
}

sub chkMustEnv
{
    my($silence) = @_;		# 获得传入参数

    my @lackList;
    showMessageLn("运行环境检查...\n--------------------------") if (!$silence);
    foreach my $bin (values(%binListHashMap)) {
        $msg = -f $bin ? '已安装' : '缺少';
        showMessageLn($bin . "\t" . $msg) if (!$silence);
        push(@lackList, $bin) if (!-f $bin);
    }
    showMessageLn('') if (!$silence);

    $size = @lackList;
    return ($size < 1);
}

sub Usage {
    warn << "__END_OF_USAGE";
=======================================================================
使用说明:    mk [选项] 编译动作 [模块|路径]

选项:
  -p, -project  : 项目名称。当 new 编译后，可以省略此选项。
  -m, -buildMode=(u|user|d|userdebug|e|eng)
                : 编译模式，默认：userdebug
  -d=n(m|h), -delay=n(m|h) 
                : 延时多少分钟/小时编译，n 为整数
                  示例：-d=30m 为延时 30 分钟，-d=2h 为延时 2 小时
  -gitpull      : 编译前先执行 git pull，但出错为停止继续编译
  -t, -tee      : 输出调试信息
  -o, -opt=参数 : 传递额外的参数，多个参数以“,”分隔
  -h, -help     : 显示使用说明

编译动作, 可用 "," 连接多个动作:
  listp, listproject
                : 列出可用项目。传递关键字参数，可以筛选出需要的项目名称
  n, new        : 全新编译
  c, clean      : 清除编译的文件，可指定模块
  r, remake     : 编译涉及修改的文件，可指定模块
  mm            : 编译某个目录，必须指定路径
  snod          : 重新打包 system.img, 不执行编译，只与 mm 动作配合使用
  update-api    : 更新 API。在 framework API 改动之后，需要首先执行此命令
  p, package    : 只打包编译结果文件，并发布到预设目录，不执行编译
  ota           : 生成 OTA 文件
  check-env, chk-env
                : 检查编译环境

模块:
  systemimage    : 生成 system.img
  bootimage      : 生成 boot.img
  recoveryimage  : 生成 recovery.img
  userdataimage  : 生成 userdata.img
  secroimage     : 生成 secro.img
  cacheimage     : 生成 cache.img

  Android.mk 中 LOCAL_PACKAGE_NAME 指定的名称

其它工具:
  prebuilts/misc/linux-x86/ccache/ccache -M 10G
                : 设置 CCACHE 缓冲池大小为 10GB

示例:
  mk listp      : 列出所有可用的项目名称。
  mk listp 320  : 列出包括 "320" 字符的项目名称。
  mk -p=prjABC -m=user n
                : 执行全新编译，项目为 prjABC，编译模式为 user。
  mk r          : 编译涉及修改的文件，只对 new 编译后有效。
  mk r,ota,p    : 编译涉及修改的文件，生成 OTA 文件，
                  并打包发布编译结果文件，并发布到预设目录。
  mk ota,p      : 生成 OTA 文件，并打包发布编译结果文件，并发布到预设目录。
  mk c,r,snod Settings
                : 清除并编译 Settings 模块（LOCAL_PACKAGE_NAME 名称）,
                  并重新打包 system.img。
  mk mm packages/apps/Settings
                : 编译指定目录。
  mk mm packages/apps/Settings snod
                : 编译指定目录，并重新打包 system.img。
__END_OF_USAGE

    exit 1;
}
