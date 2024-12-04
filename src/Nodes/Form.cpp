#include "Form.h"
#include "../Memory/Memory.h"
#include "../App.h"
#include "../Interface.h"
#include "Field.h"
#include "CheckBox.h"
#include "Select.h"

#include "../Bookmarks.h"

Node* FormNode::Construct(Allocator& allocator)
{
	FormNode::Data* data = allocator.Alloc<FormNode::Data>();
	if (data)
	{
		return allocator.Alloc<Node>(Node::Form, data);
	}

	return nullptr;
}

void FormNode::AppendParameter(char* address, const char* name, const char* value, int& numParams)
{
	if (!name)
		return;

	if (numParams == 0)
	{
		strcat(address, "?");
	}
	else
	{
		strcat(address, "&");
	}
	strcat(address, name);
	strcat(address, "=");
	if (value)
	{
		strcat(address, value);
	}
	numParams++;

}

void FormNode::BuildAddressParameterList(Node* node, char* address, int& numParams)
{
	switch(node->type)
	{
		case Node::TextField:
		{
			TextFieldNode::Data* fieldData = static_cast<TextFieldNode::Data*>(node->data);

			if (fieldData->name && fieldData->buffer)
			{
				AppendParameter(address, fieldData->name, fieldData->buffer, numParams);
			}
		}
		break;
		case Node::CheckBox:
		{
			CheckBoxNode::Data* checkboxData = static_cast<CheckBoxNode::Data*>(node->data);

			if (checkboxData && checkboxData->isChecked && checkboxData->name && checkboxData->value)
			{
				AppendParameter(address, checkboxData->name, checkboxData->value, numParams);
			}
		}
		break;
		case Node::Select:
		{
			SelectNode::Data* selectData = static_cast<SelectNode::Data*>(node->data);

			if (selectData && selectData->selected)
			{
				AppendParameter(address, selectData->name, selectData->selected->text, numParams);
			}
		}
		break;
	}

	for (node = node->firstChild; node; node = node->next)
	{
		BuildAddressParameterList(node, address, numParams);
	}
}

void FormNode::SubmitForm(Node* node)
{
	FormNode::Data* data = static_cast<FormNode::Data*>(node->data);
	App& app = App::Get();

	if (data->method == FormNode::Data::Get)
	{
		char* address = app.ui.addressBarURL.url;
		if (data->action)
		{
			strcpy(address, data->action);
		}
		int numParams = 0;

		// Remove anything after existing ?
		char* questionMark = strstr(address, "?");
		if (questionMark)
		{
			*questionMark = '\0';
		}

		BuildAddressParameterList(node, address, numParams);

		// Replace any spaces with +
		for (char* p = address; *p; p++)
		{
			if (*p == ' ')
			{
				*p = '+';
			}
		}

		app.OpenURL(URL::GenerateFromRelative(app.page.pageURL.url, address).url);
	}
	else if(data->method == FormNode::Data::Internal)
	{
		char* address = app.ui.addressBarURL.url;
		if(strstr(address, "settings"))
		{
			ProcessSettingsForm(node);
			Platform::SaveConfig();
		}
		else if(strstr(address, "bookmarks"))
		{
			ProcessBookmarksForm(node);
		}
		app.ReloadPage();
	}
}

void FormNode::ProcessSettingsForm(Node* node)
{
	switch(node->type)
	{
		case Node::TextField:
		{
			TextFieldNode::Data* fieldData = static_cast<TextFieldNode::Data*>(node->data);
			if(strcmp(fieldData->name, "cache_size") == 0)
			{
				Platform::config.cacheSize = atoi(fieldData->buffer);
			}
			else if(strcmp(fieldData->name, "cache_path") == 0)
			{
				strncpy(Platform::config.cachePath, fieldData->buffer, _MAX_PATH);
			}
			break;
		}
		case Node::CheckBox:
		{
			CheckBoxNode::Data* checkboxData = static_cast<CheckBoxNode::Data*>(node->data);
			if(strcmp(checkboxData->name, "cache_enabled") == 0)
			{
				Platform::config.enableCache = checkboxData->isChecked;
			}
			break;
		}
		case Node::Select:{
			SelectNode::Data* selectData = static_cast<SelectNode::Data*>(node->data);
			if(strcmp(selectData->name, "video_mode") == 0)
			{
				Platform::config.vidMode = atoi(selectData->selected->value);
			}
			break;
		}
	}

	for (Node* childNode = node->firstChild; childNode; childNode = childNode->next)
	{
		ProcessSettingsForm(childNode);
	}
}

void FormNode::ProcessBookmarksForm(Node* node)
{
	static const char* bookmarkTitle = NULL;
	static const char* bookmarkUrl = NULL;

	switch(node->type)
	{
		case Node::TextField:
		{
			TextFieldNode::Data* fieldData = static_cast<TextFieldNode::Data*>(node->data);
			if(strcmp(fieldData->name, "bookmark_title") == 0)
			{
				bookmarkTitle = fieldData->buffer;
			}
			else if(strcmp(fieldData->name, "bookmark_url") == 0)
			{
				bookmarkUrl = fieldData->buffer;
			}
			break;
		}
		case Node::CheckBox:
		{
			CheckBoxNode::Data* checkboxData = static_cast<CheckBoxNode::Data*>(node->data);
			if(checkboxData->isChecked && strcmp(checkboxData->name, "delete") == 0)
			{
				int index = atoi(checkboxData->value);
				DeleteBookmark(index);
			}
			break;
		}
	}

	if(bookmarkTitle && bookmarkUrl)
	{
		AddBookmark(bookmarkTitle, bookmarkUrl);
		bookmarkTitle = NULL;
		bookmarkUrl = NULL;
	}

	for (Node* childNode = node->firstChild; childNode; childNode = childNode->next)
	{
		ProcessBookmarksForm(childNode);
	}
}

void FormNode::OnSubmitButtonPressed(Node* node)
{
	Node* formNode = node->FindParentOfType(Node::Form);
	if (formNode)
	{
		SubmitForm(formNode);
	}
}
