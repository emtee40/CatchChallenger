#ifndef CATCHCHALLENGER_NOAUDIO
#include "Audio.h"
#include "PlatformMacro.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/cpp11addition.h"
#include <QCoreApplication>
#include <QSettings>
#include <iostream>

Audio Audio::audio;

Audio::Audio()
{
    QSettings settings;
    if(!settings.contains(QStringLiteral("audio_init")) || settings.value(QStringLiteral("audio_init")).toInt()==2)
    {
        settings.setValue(QStringLiteral("audio_init"),1);
        settings.sync();

        //init audio here
        m_format.setSampleRate(48000);
        m_format.setChannelCount(2);
        m_format.setSampleSize(16);
        m_format.setCodec("audio/pcm");
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleType(QAudioFormat::SignedInt);

        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        if (!info.isFormatSupported(m_format)) {
            std::cerr << "raw audio format not supported by backend, cannot play audio." << std::endl;
            return;
        }

        settings.setValue(QStringLiteral("audio_init"),2);
    }
    else
        std::cerr << "Audio disabled due to previous crash" << std::endl;
}

QAudioFormat Audio::format() const
{
    return m_format;
}

void Audio::setVolume(const int &volume)
{
    std::cout << "Audio volume set to: " << volume << std::endl;
    unsigned int index=0;
    while(index<playerList.size())
    {
        playerList.at(index)->setVolume((qreal)volume/100);
        index++;
    }
    this->volume=volume;
}

void Audio::addPlayer(QAudioOutput * const player)
{
    if(vectorcontainsAtLeastOne(playerList,player))
        return;
    playerList.push_back(player);
    player->setVolume((qreal)volume/100);
}

void Audio::setPlayerVolume(QAudioOutput * const player)
{
    player->setVolume((qreal)volume/100);
}

void Audio::removePlayer(QAudioOutput * const player)
{
    vectorremoveOne(playerList,player);
}

QStringList Audio::output_list()
{
    QStringList outputs;
    /*libvlc_audio_output_device_t * output=libvlc_audio_output_device_list_get(vlcInstance,NULL);
    do
    {
        outputs << output->psz_device;
        output=output->p_next;
    } while(output!=NULL);*/
    return outputs;
}

Audio::~Audio()
{
}

bool Audio::decodeOpus(const std::string &filePath,QByteArray &data)
{
    QBuffer buffer(&data);
    buffer.open(QBuffer::ReadWrite);

    int           ret;
    OggOpusFile  *of=op_open_file(filePath.c_str(),&ret);
    if(of==NULL) {
        fprintf(stderr,"Failed to open file '%s': %i\n","file.opus",ret);
        return false;
    }
    ogg_int64_t pcm_offset;
    ogg_int64_t nsamples;
    nsamples=0;
    pcm_offset=op_pcm_tell(of);
    if(pcm_offset!=0)
        fprintf(stderr,"Non-zero starting PCM offset: %li\n",(long)pcm_offset);
    for(;;) {
        ogg_int64_t   next_pcm_offset;
        opus_int16    pcm[120*48*2];
        unsigned char out[120*48*2*2];
        int           si;
        ret=op_read_stereo(of,pcm,sizeof(pcm)/sizeof(*pcm));
        if(ret==OP_HOLE) {
            fprintf(stderr,"\nHole detected! Corrupt file segment?\n");
            continue;
        }
        else if(ret<0) {
            fprintf(stderr,"\nError decoding '%s': %i\n","file.opus",ret);
            ret=EXIT_FAILURE;
            break;
        }
        next_pcm_offset=op_pcm_tell(of);
        pcm_offset=next_pcm_offset;
        if(ret<=0) {
            ret=EXIT_SUCCESS;
            break;
        }
        for(si=0;si<2*ret;si++) { /// Ensure the data is little-endian before writing it out.
            out[2*si+0]=(unsigned char)(pcm[si]&0xFF);
            out[2*si+1]=(unsigned char)(pcm[si]>>8&0xFF);
        }
        buffer.write(reinterpret_cast<char *>(out),sizeof(*out)*4*ret);
        nsamples+=ret;
    }
    op_free(of);

    buffer.seek(0);
    return ret==EXIT_SUCCESS;
}
#endif