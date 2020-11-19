#include <Windows.h>
#include <PluginAPI/Plugin.h>
#include <imgui/imgui.h>

#include <stdio.h>
#include <stdlib.h>
#include <array>
#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <picojson/picojson.h>

#ifdef WIN32
# define FEXPORT __declspec(dllexport)
#else
# define FEXPORT 
#endif

namespace rs
{
	class RustPlugin : public ed::IPlugin2
	{
	public:
		virtual bool Init(bool isWeb, int sedVersion) {
			m_buildLangDefinition();
			m_spv = nullptr;
			m_codegenPath = "rustc_codegen_spirv.dll";
			
			if (sedVersion == 1003005)
				m_hostVersion = 1;
			else
				m_hostVersion = GetHostIPluginMaxVersion();

			return true;
		}

		virtual void InitUI(void* ctx) {
			ImGui::SetCurrentContext((ImGuiContext*)ctx);
		}
		virtual void OnEvent(void* e) { }
		virtual void Update(float delta) {
			if (m_hostVersion >= 2 && ImGuiFileDialogIsDone("PluginRustCodegenFile")) {
				if (ImGuiFileDialogGetResult()) {
					char tempFilepath[1024] = { 0 };
					ImGuiFileDialogGetPath(tempFilepath);
					m_codegenPath = std::string(tempFilepath);
				}

				ImGuiFileDialogClose("PluginRustCodegenFile");
			}
		}
		virtual void Destroy() {
			if (m_spv)
				free(m_spv);
		}

		virtual bool IsRequired() { return 0; }
		virtual bool IsVersionCompatible(int version) { return 0; }

		virtual void BeginRender() { }
		virtual void EndRender() { }

		virtual void Project_BeginLoad() { }
		virtual void Project_EndLoad() { }
		virtual void Project_BeginSave() { }
		virtual void Project_EndSave() { }
		virtual bool Project_HasAdditionalData() { return 0; }
		virtual const char* Project_ExportAdditionalData() { return 0; }
		virtual void Project_ImportAdditionalData(const char* xml) { }
		virtual void Project_CopyFilesOnSave(const char* dir) { }

		/* list: file, newproject, project, createitem, window, custom */
		virtual bool HasCustomMenuItem() { return 0; }
		virtual bool HasMenuItems(const char* name) { return 0; }
		virtual void ShowMenuItems(const char* name) { }

		/* list: pipeline, shaderpass_add (owner = ShaderPass), pluginitem_add (owner = char* ItemType, extraData = PluginItemData) objects, editcode (owner = char* ItemName) */
		virtual bool HasContextItems(const char* name) { return 0; }
		virtual void ShowContextItems(const char* name, void* owner = nullptr, void* extraData = nullptr) { }

		// system variable methods
		virtual int SystemVariables_GetNameCount(ed::plugin::VariableType varType) { return 0; }
		virtual const char* SystemVariables_GetName(ed::plugin::VariableType varType, int index) { return 0; }
		virtual bool SystemVariables_HasLastFrame(char* name, ed::plugin::VariableType varType) { return 0; }
		virtual void SystemVariables_UpdateValue(char* data, char* name, ed::plugin::VariableType varType, bool isLastFrame) { }

		// function variables
		virtual int VariableFunctions_GetNameCount(ed::plugin::VariableType vtype) { return 0; }
		virtual const char* VariableFunctions_GetName(ed::plugin::VariableType varType, int index) { return 0; }
		virtual bool VariableFunctions_ShowArgumentEdit(char* fname, char* args, ed::plugin::VariableType vtype) { return 0; }
		virtual void VariableFunctions_UpdateValue(char* data, char* args, char* fname, ed::plugin::VariableType varType) { }
		virtual int VariableFunctions_GetArgsSize(char* fname, ed::plugin::VariableType varType) { return 0; }
		virtual void VariableFunctions_InitArguments(char* args, char* fname, ed::plugin::VariableType vtype) { }
		virtual const char* VariableFunctions_ExportArguments(char* fname, ed::plugin::VariableType vtype, char* args) { return 0; }
		virtual void VariableFunctions_ImportArguments(char* fname, ed::plugin::VariableType vtype, char* args, const char* argsString) { }

		// object manager stuff
		virtual bool Object_HasPreview(const char* type) { return 0; }
		virtual void Object_ShowPreview(const char* type, void* data, unsigned int id) { }
		virtual bool Object_IsBindable(const char* type) { return 0; }
		virtual bool Object_IsBindableUAV(const char* type) { return 0; }
		virtual void Object_Remove(const char* name, const char* type, void* data, unsigned int id) { }
		virtual bool Object_HasExtendedPreview(const char* type) { return 0; }
		virtual void Object_ShowExtendedPreview(const char* type, void* data, unsigned int id) { }
		virtual bool Object_HasProperties(const char* type) { return 0; }
		virtual void Object_ShowProperties(const char* type, void* data, unsigned int id) { }
		virtual void Object_Bind(const char* type, void* data, unsigned int id) { }
		virtual const char* Object_Export(char* type, void* data, unsigned int id) { return 0; }
		virtual void Object_Import(const char* name, const char* type, const char* argsString) { }
		virtual bool Object_HasContext(const char* type) { return 0; }
		virtual void Object_ShowContext(const char* type, void* data) { }

		// pipeline item stuff
		virtual bool PipelineItem_HasProperties(const char* type, void* data) { return 0; }
		virtual void PipelineItem_ShowProperties(const char* type, void* data) { }
		virtual bool PipelineItem_IsPickable(const char* type, void* data) { return 0; }
		virtual bool PipelineItem_HasShaders(const char* type, void* data) { return 0; }
		virtual void PipelineItem_OpenInEditor(const char* type, void* data) { }
		virtual bool PipelineItem_CanHaveChild(const char* type, void* data, ed::plugin::PipelineItemType itemType) { return 0; }
		virtual int PipelineItem_GetInputLayoutSize(const char* type, void* data) { return 0; }
		virtual void PipelineItem_GetInputLayoutItem(const char* type, void* data, int index, ed::plugin::InputLayoutItem& out) { }
		virtual void PipelineItem_Remove(const char* itemName, const char* type, void* data) { }
		virtual void PipelineItem_Rename(const char* oldName, const char* newName) { }
		virtual void PipelineItem_AddChild(const char* owner, const char* name, ed::plugin::PipelineItemType type, void* data) { }
		virtual bool PipelineItem_CanHaveChildren(const char* type, void* data) { return 0; }
		virtual void* PipelineItem_CopyData(const char* type, void* data) { return 0; }
		virtual void PipelineItem_Execute(void* Owner, ed::plugin::PipelineItemType OwnerType, const char* type, void* data) { }
		virtual void PipelineItem_Execute(const char* type, void* data, void* children, int count) { }
		virtual void PipelineItem_GetWorldMatrix(const char* type, void* data, float(&pMat)[16]) { }
		virtual bool PipelineItem_Intersect(const char* type, void* data, const float* rayOrigin, const float* rayDir, float& hitDist) { return 0; }
		virtual void PipelineItem_GetBoundingBox(const char* type, void* data, float(&minPos)[3], float(&maxPos)[3]) { }
		virtual bool PipelineItem_HasContext(const char* type, void* data) { return 0; }
		virtual void PipelineItem_ShowContext(const char* type, void* data) { }
		virtual const char* PipelineItem_Export(const char* type, void* data) { return 0; }
		virtual void* PipelineItem_Import(const char* ownerName, const char* name, const char* type, const char* argsString) { return 0; }
		virtual void PipelineItem_MoveDown(void* ownerData, const char* ownerType, const char* itemName) { }
		virtual void PipelineItem_MoveUp(void* ownerData, const char* ownerType, const char* itemName) { }
		virtual void PipelineItem_ApplyGizmoTransform(const char* type, void* data, float* transl, float* scale, float* rota) { }
		virtual void PipelineItem_GetTransform(const char* type, void* data, float* transl, float* scale, float* rota) { }
		virtual void PipelineItem_DebugVertexExecute(void* Owner, ed::plugin::PipelineItemType OwnerType, const char* type, void* data, unsigned int colorVarLoc) { }
		virtual int PipelineItem_DebugVertexExecute(const char* type, void* data, const char* childName, float rx, float ry, int vertexGroup) { return 0; }
		virtual void PipelineItem_DebugInstanceExecute(void* Owner, ed::plugin::PipelineItemType OwnerType, const char* type, void* data, unsigned int colorVarLoc) { }
		virtual int PipelineItem_DebugInstanceExecute(const char* type, void* data, const char* childName, float rx, float ry, int vertexGroup) { return 0; }
		virtual unsigned int PipelineItem_GetVBO(const char* type, void* data) { return 0; }
		virtual unsigned int PipelineItem_GetVBOStride(const char* type, void* data) { return 0; }
		virtual bool PipelineItem_CanChangeVariables(const char* type, void* data) { return 0; }
		virtual bool PipelineItem_IsDebuggable(const char* type, void* data) { return 0; }
		virtual bool PipelineItem_IsStageDebuggable(const char* type, void* data, ed::plugin::ShaderStage stage) { return 0; }
		virtual void PipelineItem_DebugExecute(const char* type, void* data, void* children, int count, int* debugID) { }
		virtual unsigned int PipelineItem_GetTopology(const char* type, void* data) { return 0; }
		virtual unsigned int PipelineItem_GetVariableCount(const char* type, void* data) { return 0; }
		virtual const char* PipelineItem_GetVariableName(const char* type, void* data, unsigned int variable) { return 0; }
		virtual ed::plugin::VariableType PipelineItem_GetVariableType(const char* type, void* data, unsigned int variable) { return ed::plugin::VariableType::Float1; }
		virtual float PipelineItem_GetVariableValueFloat(const char* type, void* data, unsigned int variable, int col, int row) { return 0; }
		virtual int PipelineItem_GetVariableValueInteger(const char* type, void* data, unsigned int variable, int col) { return 0; }
		virtual bool PipelineItem_GetVariableValueBoolean(const char* type, void* data, unsigned int variable, int col) { return 0; }
		virtual unsigned int PipelineItem_GetSPIRVSize(const char* type, void* data, ed::plugin::ShaderStage stage) { return 0; }
		virtual unsigned int* PipelineItem_GetSPIRV(const char* type, void* data, ed::plugin::ShaderStage stage) { return 0; }
		virtual void PipelineItem_DebugPrepareVariables(const char* type, void* data, const char* name) { }
		virtual bool PipelineItem_DebugUsesCustomTextures(const char* type, void* data) { return 0; }
		virtual unsigned int PipelineItem_DebugGetTexture(const char* type, void* data, int loc, const char* variableName) { return 0; }
		virtual void PipelineItem_DebugGetTextureSize(const char* type, void* data, int loc, const char* variableName, int& x, int& y, int& z) { }

		// options
		virtual bool Options_HasSection() { return true; }
		virtual void Options_RenderSection() {
			ImGui::Text("Codegen file: %s", m_codegenPath.c_str());
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 180);
			if (ImGui::Button("CHANGE", ImVec2(-1, 0))) {
				if (m_hostVersion >= 2)
					ImGuiFileDialogOpen("PluginRustCodegenFile", "Select the codegen file", "DLL file (*.dll;*.so){.dll,.so},.*");
			}
		}
		virtual void Options_Parse(const char* key, const char* val)
		{
			if (strcmp(key, "codegen") == 0) {
				m_codegenPath = std::string(val);
				if (!std::filesystem::exists(m_codegenPath)) {
					printf("[RSHADERS] codegen .dll doesn't exists.\n");
				}
			}
		}
		virtual int Options_GetCount() { return 1; } // path to codegen .dll
		virtual const char* Options_GetKey(int index)
		{
			if (index == 0)
				return "codegen";
			return nullptr;
		}
		virtual const char* Options_GetValue(int index)
		{
			if (index == 0)
				return m_codegenPath.c_str();
			return nullptr;
		}

		// languages
		virtual int CustomLanguage_GetCount() { return 1; }
		virtual const char* CustomLanguage_GetName(int langID) { return "Rust"; }
		virtual const unsigned int* CustomLanguage_CompileToSPIRV(int langID, const char* src, size_t src_len, ed::plugin::ShaderStage stage, const char* entry, ed::plugin::ShaderMacro* macros, size_t macroCount, size_t* spv_length, bool* compiled)
		{
			// write to lib.rs file
			std::ofstream writer("rust_crates/shader/src/lib.rs");
			writer << src;
			writer.close();

			m_updateEnv();

			// run cargo build
			std::string output = m_exec("cargo build --message-format=json --manifest-path rust_crates/shader/Cargo.toml -Z build-std=core --target spirv-unknown-unknown --release");


			if (output.size() == 0) {
				*compiled = false;
				AddMessage(Messages, ed::plugin::MessageType::Error, nullptr, "Cargo not properly set up", -1);
				return m_spv;
			}

			// TODO: split by \n then parse...
			bool compileSuccess = true;
			std::stringstream outputParser(output);
			std::string outputSegment = "";
			while (std::getline(outputParser, outputSegment)) {
				picojson::value v;
				std::string err = picojson::parse(v, outputSegment);
				if (!err.empty()) {
					AddMessage(Messages, ed::plugin::MessageType::Error, GetMessagesCurrentItem(Messages), "Cargo not properly set up", -1);
					*compiled = false;
					return m_spv;
				}
				if (v.is<picojson::object>()) {

					picojson::object& obj = v.get<picojson::object>();
					std::string reason = obj["reason"].get<std::string>();
					if (reason == "compiler-message") {
						picojson::object& msg = obj["message"].get<picojson::object>();

						std::string level = msg["level"].get<std::string>();
						std::string message = msg["message"].get<std::string>();

						ed::plugin::MessageType mtype = ed::plugin::MessageType::Message;
						if (level == "error")
							mtype = ed::plugin::MessageType::Error;
						else if (level == "warning")
							mtype = ed::plugin::MessageType::Warning;

						int line = -1;
						picojson::array& spans = msg["spans"].get<picojson::array>();
						for (auto& span : spans) {
							picojson::object& spanObj = span.get<picojson::object>();
							int tempLine = (int)spanObj["line_start"].get<double>();

							if (tempLine == line)
								continue;
							line = tempLine;


							AddMessage(Messages, mtype, GetMessagesCurrentItem(Messages), message.c_str(), line);
						}
						
						if (level == "error") 
							compileSuccess = false;
					}
					else if (reason == "build-finished") {
						bool status = obj["success"].get<bool>();
						if (!status)
							compileSuccess = false;
					}
				}
			}
			if (!compileSuccess) {
				*compiled = false;
				return nullptr;
			}


			// read from .spv file
			FILE* reader = fopen("rust_crates/shader/target/spirv-unknown-unknown/release/rust_shader.spv", "rb");
			fseek(reader, 0, SEEK_END);
			long fsize = ftell(reader);
			fseek(reader, 0, SEEK_SET);
			if (m_spv != nullptr)
				free(m_spv);
			m_spv = (unsigned int*)malloc((fsize/4 + 1)*4);
			fread(m_spv, 1, fsize, reader);
			fclose(reader);

			*spv_length = fsize / 4;
			*compiled = true;

			return m_spv;
		}
		virtual const char* CustomLanguage_ProcessGeneratedGLSL(int langID, const char* srcPtr)
		{
			return srcPtr;
		}
		virtual bool CustomLanguage_SupportsAutoUniforms(int langID) { return true; }
		virtual bool CustomLanguage_IsDebuggable(int langID) { return true; }
		virtual const char* CustomLanguage_GetDefaultExtension(int langID) { return "rs"; }

		// language text editor
		virtual bool ShaderEditor_Supports(int langID) { return 0; }
		virtual void ShaderEditor_Open(int langID, int editorID, const char* data, int dataLen) { }
		virtual void ShaderEditor_Render(int langID, int editorID) { }
		virtual void ShaderEditor_Close(int langID, int editorID) { }
		virtual const char* ShaderEditor_GetContent(int langID, int editorID, size_t* dataLength) { return 0; }
		virtual bool ShaderEditor_IsChanged(int langID, int editorID) { return 0; }
		virtual void ShaderEditor_ResetChangeState(int langID, int editorID) { }
		virtual bool ShaderEditor_CanUndo(int langID, int editorID) { return 0; }
		virtual bool ShaderEditor_CanRedo(int langID, int editorID) { return 0; }
		virtual void ShaderEditor_Undo(int langID, int editorID) { }
		virtual void ShaderEditor_Redo(int langID, int editorID) { }
		virtual void ShaderEditor_Cut(int langID, int editorID) { }
		virtual void ShaderEditor_Paste(int langID, int editorID) { }
		virtual void ShaderEditor_Copy(int langID, int editorID) { }
		virtual void ShaderEditor_SelectAll(int langID, int editorID) { }
		virtual bool ShaderEditor_HasStats(int langID, int editorID) { return 0; }

		// code editor
		virtual void CodeEditor_SaveItem(const char* src, int srcLen, const char* id) {}
		virtual void CodeEditor_CloseItem(const char* id) {}
		virtual bool LanguageDefinition_Exists(int id) { return true; }
		virtual int LanguageDefinition_GetKeywordCount(int id)
		{
			return m_langDefKeywords.size();
		}
		virtual const char** LanguageDefinition_GetKeywords(int id)
		{
			return m_langDefKeywords.data();
		}
		virtual int LanguageDefinition_GetTokenRegexCount(int id)
		{
			return m_langDefRegex.size();
		}
		virtual const char* LanguageDefinition_GetTokenRegex(int index, ed::plugin::TextEditorPaletteIndex& palIndex, int id)
		{
			palIndex = m_langDefRegex[index].second;
			return m_langDefRegex[index].first;
		}
		virtual int LanguageDefinition_GetIdentifierCount(int id)
		{
			return m_langDefIdentifiers.size();
		}
		virtual const char* LanguageDefinition_GetIdentifier(int index, int id)
		{
			return m_langDefIdentifiers[index].first;
		}
		virtual const char* LanguageDefinition_GetIdentifierDesc(int index, int id)
		{
			return m_langDefIdentifiers[index].second;
		}
		virtual const char* LanguageDefinition_GetCommentStart(int id)
		{
			return "/*";
		}
		virtual const char* LanguageDefinition_GetCommentEnd(int id)
		{
			return "*/";
		}
		virtual const char* LanguageDefinition_GetLineComment(int id)
		{
			return "//";
		}
		virtual bool LanguageDefinition_IsCaseSensitive(int id) { return true; }
		virtual bool LanguageDefinition_GetAutoIndent(int id) { return true; }
		virtual const char* LanguageDefinition_GetName(int id) { return "Rust"; }
		virtual const char* LanguageDefinition_GetNameAbbreviation(int id) { return "RS"; }

		// autocomplete
		virtual int Autocomplete_GetCount(ed::plugin::ShaderStage stage) { return 0; }
		virtual const char* Autocomplete_GetDisplayString(ed::plugin::ShaderStage stage, int index) { return 0; }
		virtual const char* Autocomplete_GetSearchString(ed::plugin::ShaderStage stage, int index) { return 0; }
		virtual const char* Autocomplete_GetValue(ed::plugin::ShaderStage stage, int index) { return 0; }

		// file change checks
		virtual int ShaderFilePath_GetCount() { return 0; }
		virtual const char* ShaderFilePath_Get(int index) { return 0; }
		virtual bool ShaderFilePath_HasChanged() { return 0; }
		virtual void ShaderFilePath_Update() {}

		// misc
		virtual bool HandleDropFile(const char* filename) { return 0; }
		virtual void HandleRecompile(const char* itemName) {}
		virtual void HandleRecompileFromSource(const char* itemName, int sid, const char* shaderCode, int shaderSize) {}
		virtual void HandleShortcut(const char* name) {}
		virtual void HandlePluginMessage(const char* sender, char* msg, int msgLen) {}
		virtual void HandleApplicationEvent(ed::plugin::ApplicationEvent event, void* data1, void* data2) {}
		virtual void HandleNotification(int id) {}

		// IPlugin2 stuff
		virtual bool PipelineItem_SupportsImmediateMode(const char* type, void* data, ed::plugin::ShaderStage stage) { return false; }
		virtual bool PipelineItem_HasCustomImmediateModeCompiler(const char* type, void* data, ed::plugin::ShaderStage stage) { return false; }
		virtual bool PipelineItem_ImmediateModeCompile(const char* type, void* data, ed::plugin::ShaderStage stage, const char* expression) { return false; }

		virtual unsigned int ImmediateMode_GetSPIRVSize() { return 0; }
		virtual unsigned int* ImmediateMode_GetSPIRV() { return 0; }
		virtual unsigned int ImmediateMode_GetVariableCount() { return 0; }
		virtual const char* ImmediateMode_GetVariableName(unsigned int index) { return 0; }
		virtual int ImmediateMode_GetResultID() { return 0; }

	private:
		int m_hostVersion;

		std::vector<const char*> m_langDefKeywords;
		std::vector<std::pair<const char*, ed::plugin::TextEditorPaletteIndex>> m_langDefRegex;
		std::vector<std::pair<const char*, const char*>> m_langDefIdentifiers;
		void m_buildLangDefinition()
		{
			// keywords
			m_langDefKeywords.clear();
			m_langDefKeywords = {
				"as", "break", "const", "continue",
				"crate", "else", "enum", "extern", "false",
				"fn", "rust-gpufor", "if", "impl",
				"in", "let", "loop", "match", "mod",
				"move", "mut", "pub", "ref",
				"return", "self", "Self", "static",
				"struct", "super", "trait", "true",
				"type", "unsafe", "use", "where",
				"while", "f32", "Output", "Input", "UniformConstant",
				"Vec2", "Vec3", "Vec4",
				"Mat2", "Mat3", "Mat4", "new"
			};

			// regex
			m_langDefRegex.clear();
			m_langDefRegex.push_back(std::make_pair("[ \\t]*#\\!?\\[[^\\]]*\\]", ed::plugin::TextEditorPaletteIndex::Preprocessor));
			m_langDefRegex.push_back(std::make_pair("L?\\\"(\\\\.|[^\\\"])*\\\"", ed::plugin::TextEditorPaletteIndex::String));
			m_langDefRegex.push_back(std::make_pair("\\'\\\\?[^\\']\\'", ed::plugin::TextEditorPaletteIndex::CharLiteral));
			m_langDefRegex.push_back(std::make_pair("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", ed::plugin::TextEditorPaletteIndex::Number));
			m_langDefRegex.push_back(std::make_pair("[+-]?[0-9]+[Uu]?[lL]?[lL]?", ed::plugin::TextEditorPaletteIndex::Number));
			m_langDefRegex.push_back(std::make_pair("0[0-7]+[Uu]?[lL]?[lL]?", ed::plugin::TextEditorPaletteIndex::Number));
			m_langDefRegex.push_back(std::make_pair("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", ed::plugin::TextEditorPaletteIndex::Number));
			m_langDefRegex.push_back(std::make_pair("[a-zA-Z_][a-zA-Z0-9_]*", ed::plugin::TextEditorPaletteIndex::Identifier));
			m_langDefRegex.push_back(std::make_pair("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", ed::plugin::TextEditorPaletteIndex::Punctuation));

			// identifiers
			m_langDefIdentifiers.clear();
		}

		std::string m_codegenPath;
		void m_updateEnv() {
			std::string actualPath = m_codegenPath;
			if (std::filesystem::path(m_codegenPath).is_relative())
				actualPath = (std::filesystem::current_path() / std::filesystem::path(m_codegenPath)).string();

			std::string flags = "-Z codegen-backend=" + actualPath + " -C target-feature=+glsl450";

#if defined(_WIN32)
			_putenv_s("RUSTFLAGS", flags.c_str());
#else
			setenv("RUSTFLAGS", flags.c_str(), 1);
#endif

		}
		std::string m_exec(const char* cmd) {
#if defined(_WIN32)
			std::array<char, 128> buffer;
			std::string result;
			std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
			if (!pipe) {
				printf("Failed to run exec()");
				return "";
			}
			while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
				result += buffer.data();
			}
			return result;
#else
			std::array<char, 128> buffer;
			std::string result;
			std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
			if (!pipe) {
				throw std::runtime_error("popen() failed!");
			}
			while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
				result += buffer.data();
			}
			return result;
#endif
		}


		unsigned int* m_spv;
	};
}

extern "C" {
	FEXPORT rs::RustPlugin* CreatePlugin() {
		return new rs::RustPlugin();
	}
	FEXPORT void DestroyPlugin(rs::RustPlugin* ptr) {
		delete ptr;
	}
	FEXPORT int GetPluginAPIVersion() {
		return 3;
	}
	FEXPORT int GetPluginVersion() {
		return 1;
	}
	FEXPORT const char* GetPluginName() {
		return "Rust";
	}
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}