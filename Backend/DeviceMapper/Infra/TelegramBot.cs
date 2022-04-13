using Microsoft.Extensions.Configuration;

using NetTelegramBotApi.Requests;
using NetTelegramBotApi.Types;

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DeviceMapper.Infra
{
    /// <summary>
    /// Represents a Telegram bot.
    /// </summary>
    public class TelegramBot
    {
        private readonly NetTelegramBotApi.TelegramBot _telegramBot;
        private readonly long _telegramChatID;
        private readonly string _channelName;
        private Chat _chat;
        private DateTime _lastMessageSentAt = DateTime.MinValue;
        private long _pinnedMessageID = -1;

        private GetChat _getChat
        {
            get
            {
                if (string.IsNullOrWhiteSpace(_channelName))
                    return new GetChat(_telegramChatID);
                return new GetChat(_channelName);
            }
        }

        public TelegramBot(IConfiguration configuration)
        {
            var section = configuration.GetSection("Telegram");
            _telegramBot = new NetTelegramBotApi.TelegramBot(section.GetValue<string>("Token"));
            _channelName = section.GetValue<string>("ChannelName"); ;
        }

        public TelegramBot(string telegramToken, long telegramChatID)
        {
            if (string.IsNullOrWhiteSpace(telegramToken))
                throw new ArgumentNullException(nameof(telegramToken));


            _telegramBot = new NetTelegramBotApi.TelegramBot(telegramToken);
            _telegramChatID = telegramChatID; 
        }

        public TelegramBot(string telegramToken, string channelName)
        {
            if (string.IsNullOrWhiteSpace(telegramToken))
                throw new ArgumentNullException(nameof(telegramToken));


            _telegramBot = new NetTelegramBotApi.TelegramBot(telegramToken);
            _channelName = channelName;
        }

        public async Task<Message> SendMessageAsync(string text)
        {
            if (string.IsNullOrWhiteSpace(text))
                throw new ArgumentNullException(nameof(text));

            _chat ??= await _telegramBot.MakeRequestAsync(_getChat);

            var sendMessage = default(SendMessage);
            if (string.IsNullOrWhiteSpace(_channelName))
            {
                sendMessage = new SendMessage(_telegramChatID, text);
            }
            else
            {
                sendMessage = new SendMessage(_channelName, text);
            }
            
            return await _telegramBot.MakeRequestAsync(sendMessage);
        }

        private async Task<Message> SendMessageIfSoonEnoughAsync(string text)
        {
            var message = default(Message);
            if (DateTime.Now - _lastMessageSentAt > TimeSpan.FromSeconds(1))
            {
                message = await SendMessageAsync(text);
                _lastMessageSentAt = DateTime.Now;
            }
            return message;
        }

        Dictionary<int, string> _lineNumberDictionary = new Dictionary<int, string>();
        Dictionary<int, List<string>> _lineNumberListDictionary = new Dictionary<int, List<string>>();

        public async Task<Message> CreateOrEditLastMessageAsync(string text, int lineOrGroupNumber, bool useList = false)
        {
            if (_pinnedMessageID == -1)
            {
                await Task.Delay(TimeSpan.FromSeconds(1));
                var message = await SendMessageAsync(text);
                _pinnedMessageID = message.MessageId;
            }
            if (DateTime.Now - _lastMessageSentAt > TimeSpan.FromSeconds(1))
            {
                _lastMessageSentAt = DateTime.Now;
                if (useList)
                {
                    if (!_lineNumberListDictionary.ContainsKey(lineOrGroupNumber) || _lineNumberListDictionary[lineOrGroupNumber].Count >= 10)
                    {
                        _lineNumberListDictionary[lineOrGroupNumber] = new List<string>();
                    }
                    _lineNumberListDictionary[lineOrGroupNumber].Add(text);
                    return await EditMessageAsync(_pinnedMessageID, string.Join("\n\n", _lineNumberListDictionary.OrderBy(x => x.Key).Select(x => string.Join("\n", x.Value))));
                }
                else
                {
                    _lineNumberDictionary[lineOrGroupNumber] = text;
                    return await EditMessageAsync(_pinnedMessageID, string.Join("\n\n", _lineNumberDictionary.OrderBy(x => x.Key).Select(x => string.Join("\n", x.Value))));
                }
            }
            return null;
        }

        public async Task<Message> EditMessageAsync(long messageID, string text)
        {
            _chat ??= await _telegramBot.MakeRequestAsync(_getChat);


            var editMessage = default(EditMessageText);
            if (string.IsNullOrWhiteSpace(_channelName))
            {
                editMessage = new EditMessageText(_telegramChatID, messageID, text);
            }
            else
            {
                editMessage = new EditMessageText(_channelName, messageID, text);
            }

            return await _telegramBot.MakeRequestAsync(editMessage);
        }
    }
}
