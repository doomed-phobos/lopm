#include "src/app_cli_commands.hpp"

#include "src/app_flags.hpp"
#include "src/fs.hpp"
#include "src/parse_exception.hpp"
#include "src/output.hpp"

#include <algorithm>

namespace app {
   class MissingRequiredOption : public ParseException {
   public:
      MissingRequiredOption(const Option& o) :
         ParseException("'" + o.name() + "' option is required!") {}
   };
   
   AppCliCommands::CallableOption::CallableOption(Option& option, Callback&& callback) :
      m_option(option),
      m_callback(std::move(callback)) {
      global_unique_opts.push_back(this);
   }
   AppCliCommands::CallableOption::~CallableOption() {
      global_unique_opts.erase(
         std::find_if(global_unique_opts.cbegin(), global_unique_opts.cend(), [this] (const CallableOption* uo) -> bool {
            return this == uo;
         })
      );
   }

   void AppCliCommands::CallableOption::setCallback(Callback&& callback) {
      m_callback = std::move(callback);
   }

   bool AppCliCommands::CallableOption::FindAndExecute(const PO& po) {
      for(const auto& option : global_unique_opts) {
         if(po.enabled(option->m_option) && option->m_callback != nullptr) {
            option->m_callback(option->m_option);
            return true;
         }
      }

      return false;
   }

   AppCliCommands::AppCliCommands() :
      m_uninstall(m_po.add("uninstall").setAliasChr('u').setValueName("pkg_name").setDescription("Uninstall a package.")),
      m_symLink(m_po.add("sym_link").setAliasChr('s').setDescription("Create symlinks instead copy files.")),
      m_list(m_po.add("list").setUniqueOption(true).setAliasChr('l').setDescription("Show all installed packages.")),
      m_verbose(m_po.add("verbose").setAliasChr('v').setDescription("Show what is going on.")),
      m_name(m_po.add("name").setAliasChr('n').setValueName("pkg_name").setDescription("Set custom package name, otherwise, the package name will be <input_dir> directory name.")),
      m_inputDir(m_po.add("input_dir").setAliasChr('i').setValueName("dir").setDescription("Directory where project that you want install is located.")),
      m_outputDir(m_po.add("output_dir").setAliasChr('o').setValueName("dir").setDescription("Directory where your project will install.")),
      m_help(m_po.add("help").setUniqueOption(true).setAliasChr('?').setDescription("Show this help list.")) {
   }

   void AppCliCommands::setOnHelp(Callback&& callback) {m_help.setCallback(std::move(callback));}
   void AppCliCommands::setOnShowList(Callback&& callback) {m_list.setCallback(std::move(callback));}
   void AppCliCommands::setOnUninstall(Callback&& callback) {m_uninstall.setCallback(std::move(callback));}

   std::optional<
      std::tuple<AppFlags, fs::path, fs::path, std::optional<std::string>>> AppCliCommands::parse(int argc, char* argv[]) {
      m_po.parse(argc, argv);
      if(m_po.enabled(m_verbose))
         Output::SetInstance(new VerboseOutput);

      if(CallableOption::FindAndExecute(m_po))
         return {};
      
      checkRequiredOptions();
      
      std::optional<std::string> name{};
      if(m_po.enabled(m_name))
         name = m_po.valueOf(m_name);
      return std::tuple<AppFlags, fs::path, fs::path, std::optional<std::string>>(
         parseFlags(),
         m_po.valueOf(m_inputDir),
         m_po.valueOf(m_outputDir),
         name);
   }

   AppFlags AppCliCommands::parseFlags() {
      AppFlags flags;
      if(m_po.enabled(m_symLink))
         flags.addFlags(AppFlags::kSymLink);

      return flags;
   }

   void AppCliCommands::checkRequiredOptions() {
      if(!m_po.enabled(m_inputDir)) throw MissingRequiredOption(m_inputDir);
      if(!m_po.enabled(m_outputDir)) throw MissingRequiredOption(m_outputDir);
   }
} // namespace app